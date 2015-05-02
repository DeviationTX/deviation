
#include "devofs.h"
#include <fcntl.h>
#include <assert.h>
#include <string.h>
enum {
    START_SECTOR_ID = 0xFF,
    SECTOR_SIZE     = 4096,
    SECTOR_COUNT    = 16,
    BUF_SIZE        = 100,
};

#define FILE_SIZE(x) ((x).type == FILEOBJ_DIR ? 0 : (((x).size1 << 8) | (x).size2))
#define FILE_ID(x) ((x).size1)
/* This assumes flash reset = 0x00.  bits are defined to ensure only 1 type can e set before a reset happens */
enum {
    FILEOBJ_NONE    = 0x00,
    FILEOBJ_FILE    = 0xf7,
    FILEOBJ_DIR     = 0x7f,
    FILEOBJ_DELETED = 0xff,
};
static FATFS *_fs;

int _compact()
{
    assert(0);
}

int _read(void * buf, int addr, int len)
{
    //FIXME  Need to properly handle sector crossings here
    int sector = addr / SECTOR_SIZE;
    int offset = addr % SECTOR_SIZE;
    while(len) {
        if (offset + len > SECTOR_SIZE) {
            int bytes = SECTOR_SIZE - offset;
            disk_readp(buf, sector, offset, bytes);
            buf += bytes;
            len -= bytes;
            offset = 1;
            sector++;
        } else {
            disk_readp(buf, sector, offset, len);
            len = 0;
        }
    }
    return FR_OK;
}
int _write(const void* buf, int addr, int len)
{
    //FIXME  Need to properly handle sector crossings here
    int sector = addr / SECTOR_SIZE;
    int offset = addr % SECTOR_SIZE;
    while(len) {
        if (offset + len > SECTOR_SIZE) {
            int bytes = SECTOR_SIZE - offset;
            disk_writep_rand(buf, sector, offset, bytes);
            buf += bytes;
            len -= bytes;
            offset = 1;
            sector++;
            u8 sector_id = 2;
            //write sector_id
            disk_writep_rand(&sector_id, sector, 0, 1);
        } else {
            disk_readp(buf, sector, offset, len);
            len = 0;
        }
    }
    return FR_OK;
}

void _get_next_fileobj()
{
    _fs->file_addr = _get_addr(_fs->file_addr, sizeof(struct file_header) + FILE_SIZE(_fs->file_header));
}

//search each sector for either a single start-sector or 2 back-to-back start sectors
int _find_start_sector(int *recovery_sector)
{
    unsigned i;
    int start[2] = {-1, -1};
    int start_ptr = 0;
    if (recovery_sector) {
        *recovery_sector = -1;
    }
    for (i = 0; i < SECTOR_COUNT; i++) {
        u8 id;
        disk_readp(&id, i, 0, 1);
        if (id == START_SECTOR_ID) {
            start[start_ptr++] = i;
            if (start_ptr == 2) {
                break;
            }
        }
    }
    if (start_ptr == 1) {
        return start[0];
    }
    if (start_ptr == 0) {
        return -1;
    }
    if(start[0] == 0 && start[1] == SECTOR_COUNT-1) {
        start[0] = 15; start[1] = 0;
    }
    if (recovery_sector) {
        *recovery_sector = start[1];
    }
    return start[0];
}
/* Mount/Unmount a logical drive */
FRESULT pf_mount (FATFS* fs)
{
    _fs = fs;
    int recovery_sector;
    fs->file_addr = -1;
    fs->file_cur_pos = -1;
    fs->parent_dir = 0;
    disk_initialize();
    fs->start_sector = _find_start_sector(&fs->compact_sector);
    if (fs->start_sector  == -1) {
        return FR_NO_FILESYSTEM;
    }
    if (fs->compact_sector >= 0) {
        return _compact(fs);
    }
    fs->compact_sector = fs->start_sector == 0 ? SECTOR_COUNT-1 : fs->start_sector-1;
}

int _expand_chars(char *dest, const char *src, int len)
{
   int i, j;
   for(i = 0; i < len; i++) {
       if(src[i] == 0 || src[i] == '.') {
           for(j = i; j < len; j++) {
               dest[j] = 0;
           }
           break;
       }
       dest[i] = src[i];
   }
   return i;
}

FRESULT _find_file(const char *fullname)
{
   char name[11];
   int i, j;

   int len = strlen(fullname);
   memset(name, 0, 11);
   for (i = 0, j = 0; i < len; i++, j++) {
       if(fullname[i] == 0)
           break;
       if(fullname[i] == '.') {
          j = 7;
       } else {
          name[j] = fullname[i];
       }
   }
   int start = _fs->start_sector * SECTOR_SIZE + 1;
   _read(&_fs->file_header, start, sizeof(struct file_header));

   while(_fs->file_header.type != FILEOBJ_NONE) {
       if (_fs->parent_dir == _fs->file_header.parent_dir && strncmp(_fs->file_header.name, name, 11) == 0) {
           //Found matching file
           _fs->file_addr = start;
           _fs->file_cur_pos = -1;
           return FR_OK;
       }
       start = _get_addr(start, sizeof(struct file_header) + FILE_SIZE(_fs->file_header));
       _read(&_fs->file_header, start, sizeof(struct file_header));
   }
   return FR_NO_PATH;
}

FRESULT pf_opendir (DIR *dir, const char *name)
{
    int res = _find_file(name);
    if (res)
        return res;
    if (_fs->file_header.type == FILEOBJ_DIR) {
        _fs->parent_dir = FILE_ID(_fs->file_header);
        return FR_OK;
    }
    _fs->file_addr = -1;
    return FR_NO_FILE;
}

FRESULT pf_readdir (DIR *dir, FILINFO *fi)
{
    if (_fs->file_addr == -1) {
        return FR_NO_FILE;
    }
    if (_fs->file_cur_pos == -1) {
        //Start at the beginning
        _fs->file_addr = _fs->start_sector + 1;
        _fs->file_cur_pos = 0;
    } else {
        if (_fs->file_header.type == FILEOBJ_NONE)
            return FR_NO_PATH;
        _get_next_fileobj();
    }
    _read(&_fs->file_header, _fs->file_addr, sizeof(struct file_header));
    while (_fs->file_header.type != FILEOBJ_NONE) {
        if (_fs->file_header.parent_dir == _fs->parent_dir) {
            //Found next object
            fi->fname[9] = 0;
            int len = _expand_chars(fi->fname, _fs->file_header.name, 8);
            fi->fname[len++] = '.';
            _expand_chars(fi->fname+len, _fs->file_header.name + 8, 3);
            fi->fattrib = (_fs->file_header.type == FILEOBJ_DIR) ? AM_DIR : 0;
            fi->fname[12] = 0;
            return FR_OK;
        }
        _get_next_fileobj();
        _read(&_fs->file_header, _fs->file_addr, sizeof(struct file_header));
    }
    return FR_NO_PATH;
}

int _get_next_write_addr()
{
    struct file_header fh = _fs->file_header;
    int addr = _fs->file_addr;
    while(fh.type != FILEOBJ_NONE) {
        addr = _get_addr(addr, sizeof(struct file_header) + FILE_SIZE(fh));
        _read(&fh, addr, sizeof(struct file_header));
    }
    return addr;
}

void _create_empty_file()
{
    //Delete file 1st
    u8 data[BUF_SIZE];
    data[0] = FILEOBJ_DELETED;
    disk_writep_rand(data, _fs->file_addr / 4096, _fs->file_addr % 4096, 1);
    _fs->file_addr = _get_next_write_addr();
    int end_addr = _get_addr(_fs->file_addr, sizeof(struct file_header) + FILE_SIZE(_fs->file_header));
    if (end_addr >= _fs->compact_sector*SECTOR_SIZE) {
        //file won't fit.  need to compact
        _compact();
    }
    //duplicate file header to new location
    _write(&_fs->file_header, _fs->file_addr, sizeof(struct file_header));
    _fs->file_cur_pos = 0;
}

FRESULT pf_open (const char *name, unsigned flags)
{
    char cur_dir[13];
    int i = 0;
    _fs->parent_dir = 0;
    int cur_idx = 0;
    while(1) {
        if (name[i] == '/') {
            cur_dir[cur_idx] = 0;
            if (_find_file(cur_dir) != FR_OK || _fs->file_header.type != FILEOBJ_DIR) {
                return FR_NO_PATH;
            }
            _fs->parent_dir = FILE_ID(_fs->file_header);
            cur_idx = 0;
        } else {
            cur_dir[cur_idx++] = name[i];
            if (name[i] == 0) {
                break;
            }
        }
        i++;
    }
    int res = _find_file(cur_dir);
    if (res)
        return res;
    if (_fs->file_header.type == FILEOBJ_FILE) {
        _fs->file_cur_pos = 0;
        if (flags && O_WRONLY) {
            _create_empty_file();
        }
        return FR_OK;
    }
    return FR_NO_FILE;
}

FRESULT pf_read (void *buf, u16 requested, u16 *actual)
{
    if (requested + _fs->file_cur_pos > FILE_SIZE(_fs->file_header)) {
        requested = FILE_SIZE(_fs->file_header) - _fs->file_cur_pos;
    } 
    _read(buf, _get_addr(_fs->file_addr, sizeof(struct file_header) + _fs->file_cur_pos), requested);
    _fs->file_cur_pos += requested;
    *actual = requested;
    return FR_OK;
}

int _get_addr(int addr, int offset)
{
    int base_addr  = addr / SECTOR_SIZE;
    int sec_offset = addr % SECTOR_SIZE;
    while(sec_offset + offset > SECTOR_SIZE) {
        base_addr++;
        sec_offset++; // account for sector header byte
        if (sec_offset == SECTOR_SIZE) { //account for edge-of-sector case
            base_addr++;
            sec_offset = 1;
        }
        offset -= SECTOR_SIZE;
    }
    sec_offset += offset;
    addr = base_addr * SECTOR_SIZE + sec_offset;
    return addr % (SECTOR_COUNT * SECTOR_SIZE); //wrap
}

FRESULT pf_lseek (u32 pos) {
    if (pos > FILE_SIZE(_fs->file_header)) {
        return FR_DISK_ERR;
    }
    _fs->file_cur_pos = pos;
    return FR_OK;
}
FRESULT pf_write (const void *buffer, u16 requested, u16 *written)
{
    if (requested + _fs->file_cur_pos > FILE_SIZE(_fs->file_header)) {
        requested = FILE_SIZE(_fs->file_header) - _fs->file_cur_pos;
    } 
    _write(buffer, _get_addr(_fs->file_addr, sizeof(struct file_header) + _fs->file_cur_pos), requested);
    _fs->file_cur_pos += requested;
    *written = requested;
    return FR_OK;  
}

FRESULT pf_switchfile (FATFS *);
FRESULT pf_maximize_file_size();
