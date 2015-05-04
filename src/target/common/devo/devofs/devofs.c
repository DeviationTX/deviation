
#include "devofs.h"
#include <fcntl.h>
#include <assert.h>
#include <string.h>

#include "devofs_io.h"
enum {
    SECTOR_SIZE     = 4096,
    SECTOR_COUNT    = 16,
    BUF_SIZE        = 100,
};

enum {
    SECTORID_START = 0xFF,
    SECTORID_EMPTR = 0x00,
    SECTORID_DATA  = 0x02,
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

static int _read(void * buf, int addr, int len);
static int _get_addr(int addr, int offset);

int _get_next_sector(int sec) {
    return (sec + 1) % SECTOR_COUNT;
}

void _write_sector_id(int sector, u8 id) {
    disk_writep_rand(&id, sector, 0, 1);
}

FRESULT df_compact()
{
    u8 buf[BUF_SIZE];
    u8 *buf_ptr;
    struct file_header fh;
    int read_addr = _fs->start_sector * SECTOR_SIZE + 1;
    int write_sec = _fs->compact_sector;
    int file_addr = _fs->file_addr;
    int write_off = 1;
    int buf_len;
    disk_erasep(write_sec);
    _write_sector_id(write_sec, SECTORID_START);
    while(1) {
        // process one file/directory
        if (read_addr == file_addr)
            file_addr = write_sec * SECTOR_SIZE + write_off;

        _read(&fh, read_addr, sizeof(struct file_header));
        if (fh.type == FILEOBJ_NONE)
            break;
        int len = FILE_SIZE(fh);
        if (fh.type == FILEOBJ_DELETED) {
            read_addr = _get_addr(read_addr, sizeof(struct file_header) + len);
            continue;
        }
        read_addr = _get_addr(read_addr, sizeof(struct file_header));
        //write header/directory-entry
        memcpy(buf, &fh, sizeof(struct file_header));
        buf_len = sizeof(struct file_header);
        buf_ptr = buf;
        while(buf_len) {
            int sector_overwrite = (write_off + buf_len >= SECTOR_SIZE);
            if (sector_overwrite){
                //write to end of sector
                int sector_buf_len = SECTOR_SIZE - write_off;
                disk_writep_rand(buf_ptr, write_sec, write_off, sector_buf_len);
                buf_len -= sector_buf_len;
                buf_ptr += sector_buf_len;
                write_sec = _get_next_sector(write_sec);
                write_off = 1;
                disk_erasep(write_sec);
                _write_sector_id(write_sec, SECTORID_DATA);
                if (buf_len)
                    continue;
            } else {
                //write whole buffer
                disk_writep_rand(buf_ptr, write_sec, write_off, buf_len);
                write_off += buf_len;
            }
            if (! len)
                break;
            buf_len = len > BUF_SIZE ? BUF_SIZE : len;
            _read(buf, read_addr, buf_len);
            buf_ptr = buf;
            len -= buf_len;
            read_addr = _get_addr(read_addr, buf_len);
        }
    }
    //erase remaining sectors
    int last_sec = write_sec;
    write_sec = _get_next_sector(write_sec);
    while (write_sec != _fs->compact_sector) {
        disk_erasep(write_sec);
        last_sec = write_sec;
        write_sec = _get_next_sector(write_sec);
    }
    //update _fs
    _fs->start_sector = _fs->compact_sector;
    _fs->compact_sector = last_sec;
    _fs->file_addr = file_addr;
    return FR_OK;
}

int _read(void * buf, int addr, int len)
{
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
            _write_sector_id(sector, SECTORID_DATA);
            //write sector_id
        } else {
            disk_writep_rand(buf, sector, offset, len);
            len = 0;
        }
    }
    return FR_OK;
}

void _get_next_fileobj(FATFS *fs)
{
    fs->file_addr = _get_addr(fs->file_addr, sizeof(struct file_header) + FILE_SIZE(fs->file_header));
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
        if (id == SECTORID_START) {
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
FRESULT df_mount (FATFS* fs)
{
    _fs = fs;
    fs->file_addr = -1;
    fs->file_cur_pos = -1;
    fs->parent_dir = 0;
    disk_initialize();
    fs->start_sector = _find_start_sector(&fs->compact_sector);
    if (fs->start_sector  == -1) {
        return FR_NO_FILESYSTEM;
    }
    if (fs->compact_sector >= 0) {
        return df_compact(fs);
    }
    fs->compact_sector = fs->start_sector == 0 ? SECTOR_COUNT-1 : fs->start_sector-1;
    return FR_OK;
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

FRESULT _find_file(FATFS *fs, const char *fullname)
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
   _read(&fs->file_header, fs->file_addr, sizeof(struct file_header));

   while(fs->file_header.type != FILEOBJ_NONE) {
       if (fs->file_header.type != FILEOBJ_DELETED && fs->parent_dir == fs->file_header.parent_dir && memcmp(fs->file_header.name, name, 11) == 0) {
           //Found matching file
           fs->file_cur_pos = -1;
           return FR_OK;
       }
       fs->file_addr = _get_addr(fs->file_addr, sizeof(struct file_header) + FILE_SIZE(fs->file_header));
       _read(&fs->file_header, fs->file_addr, sizeof(struct file_header));
   }
   return FR_NO_PATH;
}

FRESULT df_opendir (DIR *dir, const char *name)
{
    *dir = *_fs;
    dir->file_addr = _fs->start_sector * SECTOR_SIZE + 1; //reset current position
    int res = _find_file(dir, name);
    if (res)
        return res;
    if (dir->file_header.type == FILEOBJ_DIR) {
        dir->parent_dir = FILE_ID(dir->file_header);
        return FR_OK;
    }
    dir->file_addr = -1;
    return FR_NO_FILE;
}

FRESULT df_readdir (DIR *dir, FILINFO *fi)
{
    if (dir->file_addr == -1) {
        return FR_NO_FILE;
    }
    if (dir->file_cur_pos == -1) {
        //Start at the beginning
        dir->file_addr = dir->start_sector + 1;
        dir->file_cur_pos = 0;
    } else {
        if (dir->file_header.type == FILEOBJ_NONE)
            return FR_NO_PATH;
        _get_next_fileobj(dir);
    }
    _read(&dir->file_header, dir->file_addr, sizeof(struct file_header));
    while (dir->file_header.type != FILEOBJ_NONE) {
        if (dir->file_header.parent_dir == dir->parent_dir) {
            //Found next object
            fi->fname[9] = 0;
            int len = _expand_chars(fi->fname, dir->file_header.name, 8);
            fi->fname[len++] = '.';
            _expand_chars(fi->fname+len, dir->file_header.name + 8, 3);
            fi->fattrib = (dir->file_header.type == FILEOBJ_DIR) ? AM_DIR : 0;
            fi->fname[12] = 0;
            return FR_OK;
        }
        _get_next_fileobj(dir);
        _read(&dir->file_header, dir->file_addr, sizeof(struct file_header));
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
        df_compact();
    }
    //duplicate file header to new location
    _write(&_fs->file_header, _fs->file_addr, sizeof(struct file_header));
    _fs->file_cur_pos = 0;
}

FRESULT df_open (const char *name, unsigned flags)
{
    char cur_dir[13];
    int i = 0;
    _fs->parent_dir = 0;
    _fs->file_addr = _fs->start_sector * SECTOR_SIZE + 1; //reset current position
    int cur_idx = 0;
    while(1) {
        if (name[i] == '/') {
            cur_dir[cur_idx] = 0;
            if (_find_file(_fs, cur_dir) != FR_OK || _fs->file_header.type != FILEOBJ_DIR) {
                return FR_NO_PATH;
            }
            _fs->parent_dir = FILE_ID(_fs->file_header);
            _get_next_fileobj(_fs); //get the next file object
            cur_idx = 0;
        } else {
            cur_dir[cur_idx++] = name[i];
            if (name[i] == 0) {
                break;
            }
        }
        i++;
    }
    int res = _find_file(_fs, cur_dir);
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

FRESULT df_read (void *buf, u16 requested, u16 *actual)
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

FRESULT df_lseek (u32 pos) {
    if ((int)pos > FILE_SIZE(_fs->file_header)) {
        return FR_DISK_ERR;
    }
    _fs->file_cur_pos = pos;
    return FR_OK;
}
FRESULT df_write (const void *buffer, u16 requested, u16 *written)
{
    if (requested + _fs->file_cur_pos > FILE_SIZE(_fs->file_header)) {
        requested = FILE_SIZE(_fs->file_header) - _fs->file_cur_pos;
    } 
    _write(buffer, _get_addr(_fs->file_addr, sizeof(struct file_header) + _fs->file_cur_pos), requested);
    _fs->file_cur_pos += requested;
    *written = requested;
    return FR_OK;  
}

FRESULT df_switchfile (FATFS *fs)
{
    _fs = fs;
    return FR_OK;
}

FRESULT df_maximize_file_size() { return FR_OK; }
