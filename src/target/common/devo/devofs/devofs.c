
#include "devofs.h"
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>

#include "devofs_io.h"

#ifndef DEVOFS_CREATE_FILE
    #define DEVOFS_CREATE_FILE 0
#endif

enum {
    SECTOR_SIZE         = 4096,
    MINIMUM_EXTRA_BYTES = 4096,
    MINIMUM_NEW_FILE_SIZE = 8192,
    SECTOR_COUNT        = 16,
    BUF_SIZE            = 100,
};

enum {
    SECTORID_START = 0xFF,
    SECTORID_EMPTR = 0x00,
    SECTORID_DATA  = 0x02,
};

#define FILE_SIZE(x) ((x).type == FILEOBJ_DIR ? 0 : (((x).size1 << 16) | ((x).size2 << 8) | (x).size3))
#define FILE_ID(x) ((x).size1)
#define FILE_DELETED(x) (((x).type & FILEOBJ_DELMASK) == FILEOBJ_DELMASK)
/* This assumes flash reset = 0x00.  bits are defined to ensure only 1 type can e set before a reset happens */
enum {
    FILEOBJ_NONE    = 0x00,
    FILEOBJ_FILE    = 0x43,
    FILEOBJ_WRITE   = 0x03, //File is being written (this state will never be written to disk)
    FILEOBJ_DIR     = 0x41,
    FILEOBJ_FILEDEL = 0xC3,
    FILEOBJ_DIRDEL  = 0xC1,
    FILEOBJ_DELMASK = 0xC0,
};
static FATFS *_fs;

static int _spiread(void * buf, int addr, int len);
static int _get_addr(int addr, int offset);

int _get_next_sector(int sec) {
    return (sec + 1) % SECTOR_COUNT;
}

void _write_sector_id(int sector, u8 id) {
    disk_writep_rand(&id, sector, 0, 1);
}

void _fill_fileinfo(FATFS *dir, FILINFO *fi) {
    char *ptr1 = fi->fname;
    int  pos = 0;
    while(pos < 8 && dir->file_header.name[pos]) {
        *ptr1++ = dir->file_header.name[pos++];
    }
    if (dir->file_header.name[8]) {
        pos = 8;
        *ptr1++ = '.';
        while(pos < 11 && dir->file_header.name[pos]) {
            *ptr1++ = dir->file_header.name[pos++];
        }
    }
    *ptr1 = 0;
    fi->fattrib = (dir->file_header.type == FILEOBJ_DIR) ? AM_DIR : AM_FILE;
    fi->fsize = FILE_SIZE(dir->file_header);
}

FRESULT df_compact()
{
    u8 buf[BUF_SIZE];
    u8 *buf_ptr;
    struct file_header fh;
    //printf("Start: %08x %08x %08x %08x\n", _fs->start_sector, _fs->compact_sector, _fs->file_addr, _fs->file_cur_pos);
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

        _spiread(&fh, read_addr, sizeof(struct file_header));
        if (fh.type == FILEOBJ_NONE)
            break;
        int len = FILE_SIZE(fh);
        if (FILE_DELETED(fh)) {
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
            _spiread(buf, read_addr, buf_len);
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
    //printf("End: %08x %08x %08x %08x\n", _fs->start_sector, _fs->compact_sector, _fs->file_addr, _fs->file_cur_pos);
    return FR_OK;
}

int _spiread(void * buf, int addr, int len)
{
    int sector = addr / SECTOR_SIZE;
    int offset = addr % SECTOR_SIZE;
    u16 actual;
    int orig_len = len;
    while(len) {
        if (offset + len >= SECTOR_SIZE) {
            int bytes = SECTOR_SIZE - offset;
            disk_readp_cnt(buf, sector, offset, bytes, &actual);
            buf += actual;
            len -= actual;
            if (actual != bytes)
                break;
            offset = 1;
            sector = (sector + 1) % SECTOR_COUNT;
        } else {
            disk_readp_cnt(buf, sector, offset, len, &actual);
            len -= actual;
            break;
        }
    }
    return orig_len - len;
}
int _spiwrite(const void* buf, int addr, int len)
{
    int sector = addr / SECTOR_SIZE;
    int offset = addr % SECTOR_SIZE;
    while(len) {
        if (offset + len >= SECTOR_SIZE) {
            int bytes = SECTOR_SIZE - offset;
            disk_writep_rand(buf, sector, offset, bytes);
            buf += bytes;
            len -= bytes;
            offset = 1;
            sector = (sector + 1) % SECTOR_COUNT;
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

    //Must initialize file_addr and file_header in case the 1st action on the FS is a write
    fs->file_addr = _fs->start_sector * SECTOR_SIZE + 1; //reset current position
    _spiread(&fs->file_header, fs->file_addr, sizeof(struct file_header));
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

void _format_filename(const char *filename, char *expanded_name)
{
   unsigned char i, j;

   unsigned len = strlen(filename);
   memset(expanded_name, 0, 11);
   for (i = 0, j = 0; i < len; i++, j++) {
       if(filename[i] == 0)
           break;
       if(filename[i] == '.') {
          j = 7;
       } else {
          expanded_name[j] = tolower((unsigned char)filename[i]);
       }
   }
}

//void _print_header(struct file_header *header)
//{
//   char name[13];
//   memset(name, 0, sizeof(name));
//   _format_filename(header->name, name);
//    printf("%02x  %d  %13s    %d    %d\n", header->type, header->parent_dir, name, FILE_ID(*header), FILE_SIZE(*header));
//}
FRESULT _find_file(FATFS *fs, const char *fullname)
{
   char name[11];

   _format_filename(fullname, name);
   _spiread(&fs->file_header, fs->file_addr, sizeof(struct file_header));

   while(fs->file_header.type != FILEOBJ_NONE) {
       if (! FILE_DELETED(fs->file_header) && fs->parent_dir == fs->file_header.parent_dir && memcmp(fs->file_header.name, name, 11) == 0) {
           //Found matching file
           fs->file_cur_pos = -1;
           return FR_OK;
       }
       fs->file_addr = _get_addr(fs->file_addr, sizeof(struct file_header) + FILE_SIZE(fs->file_header));
       _spiread(&fs->file_header, fs->file_addr, sizeof(struct file_header));
   }
   return FR_NO_PATH;
}

int _find_parent_dir(FATFS *fs, const char *path, char *cur_dir)
{
    int i = 0;
    fs->parent_dir = 0;
    fs->file_addr = _fs->start_sector * SECTOR_SIZE + 1; //reset current position
    int cur_idx = 0;
    //Find file's owner directory
    while(1) {
        if (path[i] == '/') {
            cur_dir[cur_idx] = 0;
            if (_find_file(fs, cur_dir) != FR_OK || _fs->file_header.type != FILEOBJ_DIR) {
                return FR_NO_PATH;
            }
            fs->parent_dir = FILE_ID(fs->file_header);
            _get_next_fileobj(fs); //get the next file object
            cur_idx = 0;
        } else {
            cur_dir[cur_idx++] = path[i];
            if (path[i] == 0) {
                break;
            }
        }
        i++;
    }
    return FR_OK;
}

FRESULT df_opendir (DIR *dir, const char *name)
{
    char cur_dir[13];
    *dir = *_fs;

    // First check if this is the root directory
    dir->file_addr = _fs->start_sector * SECTOR_SIZE + 1; //reset current position
    dir->parent_dir = 0;
    if (name[0] == 0 || (name[0] == '/' && name[1] == 0)) {
        dir->file_cur_pos = -1;
        return FR_OK;
    }

    int res = _find_parent_dir(dir, name, cur_dir);
    if (res)
        return res;
    //Find the dir itself
    res = _find_file(dir, cur_dir);
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
        dir->file_addr = dir->start_sector * SECTOR_SIZE + 1; //reset current position
        dir->file_cur_pos = 0;
    } else {
        if (dir->file_header.type == FILEOBJ_NONE)
            return FR_NO_PATH;
        _get_next_fileobj(dir);
    }
    _spiread(&dir->file_header, dir->file_addr, sizeof(struct file_header));
    while (dir->file_header.type != FILEOBJ_NONE) {
        if (! FILE_DELETED(dir->file_header) && dir->file_header.parent_dir == dir->parent_dir) {
            _fill_fileinfo(dir, fi);
            return FR_OK;
        }
        _get_next_fileobj(dir);
        _spiread(&dir->file_header, dir->file_addr, sizeof(struct file_header));
    }
    return FR_NO_PATH;
}

int _get_next_write_addr()
{
    struct file_header fh = _fs->file_header;
    int addr = _fs->file_addr;
    while(fh.type != FILEOBJ_NONE) {
        addr = _get_addr(addr, sizeof(struct file_header) + FILE_SIZE(fh));
        _spiread(&fh, addr, sizeof(struct file_header));
    }
    return addr;
}

int _get_free_space()
{
    // This assues _fs->file_addr is already at the next writeable location
    int delta = _fs->compact_sector - (1 + (_fs->file_addr / SECTOR_SIZE)); //# sectors from next boundary to the compact_sector
    if (delta < 0)
        delta += SECTOR_COUNT;
    delta = delta * (SECTOR_SIZE - 1);
    delta += SECTOR_SIZE - (_fs->file_addr % SECTOR_SIZE);
    return delta - 1;
}
    
void _create_empty_file(int delete_first)
{
    //Delete file 1st
    u8 data[BUF_SIZE];
    if (delete_first) {
        data[0] = FILEOBJ_FILEDEL;
        disk_writep_rand(data, _fs->file_addr / SECTOR_SIZE, _fs->file_addr % SECTOR_SIZE, 1);
        _fs->file_addr = _get_next_write_addr();
    }

    unsigned cur_size = FILE_SIZE(_fs->file_header);
    if (cur_size == 0 ) //we need to make sure max_size is non-zero
        cur_size = 1;
    unsigned requested_size = cur_size + MINIMUM_EXTRA_BYTES;
    if (requested_size < MINIMUM_NEW_FILE_SIZE)
        requested_size = MINIMUM_NEW_FILE_SIZE;
    int max_size = _get_free_space();
    if (requested_size > max_size)
        max_size = requested_size;
    //Max size is total space available, we need to subtract the file_header
    if (max_size - cur_size > sizeof(struct file_header))
        max_size -= sizeof(struct file_header);
    
    int end_addr = _get_addr(_fs->file_addr, sizeof(struct file_header) + max_size);
    //Check whether end_addr is past the compact_sector
    if (end_addr > _fs->compact_sector*SECTOR_SIZE || end_addr < _fs->file_addr && _fs->file_addr <= _fs->compact_sector*SECTOR_SIZE) {
        //file won't fit.  need to compact
        df_compact();
        max_size = _get_free_space();
        //printf("Compacting: New max size: %d\n", max_size);
    }
    //duplicate file header to new location
    //zero file size (we'll write the actual size at close
    if(_fs->file_header.type != FILEOBJ_DIR) {
        _fs->file_header.size1 = 0; 
        _fs->file_header.size2 = 0; 
        _fs->file_header.size3 = 0; 
        _spiwrite(&_fs->file_header, _fs->file_addr, sizeof(struct file_header));
        //place the maximum allocated filesize as a place-holder
        _fs->file_header.size1 = 0xff & (max_size >> 16);
        _fs->file_header.size2 = 0xff & (max_size >> 8);
        _fs->file_header.size3 = 0xff & (max_size);
        _fs->file_header.type  = FILEOBJ_WRITE;
        _fs->file_cur_pos = 0;
    } else {
        _spiwrite(&_fs->file_header, _fs->file_addr, sizeof(struct file_header));
    }
}

FRESULT df_unlink(const char *name)
{
    char cur_dir[13];
    int res = _find_parent_dir(_fs, name, cur_dir);
    if (res)
        return res;
    res = _find_file(_fs, cur_dir);
    if (res == 0) {
        u8 data[2];
        data[0] = _fs->file_header.type |= FILEOBJ_DELMASK;
        disk_writep_rand(data, _fs->file_addr / SECTOR_SIZE, _fs->file_addr % SECTOR_SIZE, 1);
        return FR_OK;
    }
    return FR_NO_FILE;
}

void _create_file_or_dir(char *fname, int type)
{
    // Need to initialzie addr and read 1st item for get_next_write_addr
    _fs->file_addr = _fs->start_sector * SECTOR_SIZE + 1; //reset current position
    _spiread(&_fs->file_header, _fs->file_addr, sizeof(struct file_header));
    if (type == AM_FILE) {
        _fs->file_addr = _get_next_write_addr();
        _fs->file_header.type = FILEOBJ_FILE;
    } else {
        int id;
        int i;
        unsigned char seen_dir[8];
        struct file_header fh = _fs->file_header;
        memset(seen_dir, 0, 8);
        seen_dir[0] = 1;
        while(fh.type != FILEOBJ_NONE) {
            if (fh.type == FILEOBJ_DIR) {
                id = FILE_ID(fh);
                seen_dir[id / 8] |= 1 << (id % 8);
            }
            _fs->file_addr = _get_addr(_fs->file_addr, sizeof(struct file_header) + FILE_SIZE(fh));
            _spiread(&fh, _fs->file_addr, sizeof(struct file_header));
        }
        for(i = 0; i < 256; i++) {
            if((seen_dir[i/8] & (1 << (i % 8))) == 0)
               break;
        }
        if (i == 256)
            return;
        _fs->file_header.size1 = i;
        _fs->file_header.size2 = 0;
        _fs->file_header.size3 = 0;
    }
    _fs->file_header.parent_dir = _fs->parent_dir;
    _format_filename(fname, _fs->file_header.name);
    _fs->file_header.type = type == AM_DIR ? FILEOBJ_DIR : FILEOBJ_FILE;
    _fs->file_cur_pos = -1;
    _create_empty_file(0);
}

#if DEVOFS_CREATE_FILE
FRESULT df_mkdir(const char *name)
{
    char cur_dir[13];
    int res = _find_parent_dir(_fs, name, cur_dir);
    if (res)
        return res;
    if (_find_file(_fs, cur_dir) == FR_OK)
        return FR_NO_FILE;
    _create_file_or_dir(cur_dir, AM_DIR);
    return FR_OK;
}
#endif    
FRESULT df_open (const char *name, unsigned flags)
{
    char cur_dir[13];
    int res = _find_parent_dir(_fs, name, cur_dir);
    if (res)
        return res;
    //Find the file itself
    res = _find_file(_fs, cur_dir);
    if (res) {
#if DEVOFS_CREATE_FILE
        // Only enable if creating new files is required, since it pulls in a bunch of extra code
        if (flags & O_CREAT) {
            //File doesn't exist but we want to create it
            _create_file_or_dir(cur_dir, AM_FILE);
            _fs->file_cur_pos = 0;
            return FR_OK;
        }
#endif
        return res;
    }
    if (_fs->file_header.type == FILEOBJ_FILE) {
        _fs->file_cur_pos = 0;
        if (flags & O_CREAT) {
            _create_empty_file(1);
        }
        return FR_OK;
    }
    return FR_NO_FILE;
}

FRESULT df_close ()
{
    if (_fs->file_header.type == FILEOBJ_WRITE) {
        _fs->file_header.size1 = 0xff & (_fs->file_cur_pos >> 16);
        _fs->file_header.size2 = 0xff & (_fs->file_cur_pos >> 8);
        _fs->file_header.size3 = 0xff & (_fs->file_cur_pos >> 0);
        _spiwrite(&_fs->file_header.size1, _get_addr(_fs->file_addr, offsetof(struct file_header, size1)), 3);
    }
    _fs->file_cur_pos = -1;
    return FR_OK;
}

FRESULT df_read (void *buf, u16 requested, u16 *actual)
{
    if (requested + _fs->file_cur_pos > FILE_SIZE(_fs->file_header)) {
        requested = FILE_SIZE(_fs->file_header) - _fs->file_cur_pos;
    } 
    *actual = _spiread(buf, _get_addr(_fs->file_addr, sizeof(struct file_header) + _fs->file_cur_pos), requested);
    _fs->file_cur_pos += *actual;
    return FR_OK;
}

int _get_addr(int addr, int offset)
{
    int base_addr  = addr / SECTOR_SIZE;
    int sec_offset = addr % SECTOR_SIZE;
    while(sec_offset + offset >= SECTOR_SIZE) {
        base_addr++;
        sec_offset++; // account for sector header byte
        //if (sec_offset == SECTOR_SIZE) { //account for edge-of-sector case
        //    base_addr++;
        //    sec_offset = 1;
        //}
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
        //printf( "Truncating request from %d", requested);
        requested = FILE_SIZE(_fs->file_header) - _fs->file_cur_pos;
        //printf(" to %d\n", requested);
    } 
    _spiwrite(buffer, _get_addr(_fs->file_addr, sizeof(struct file_header) + _fs->file_cur_pos), requested);
    _fs->file_cur_pos += requested;
    *written = requested;
    return FR_OK;  
}

FRESULT df_stat(FILINFO *fi) {
    if(_fs->file_header.type == FILEOBJ_NONE || FILE_DELETED(_fs->file_header)) {
        return FR_NO_PATH;
    }
    _fill_fileinfo(_fs, fi);
    return FR_OK;
}
        
FRESULT df_switchfile (FATFS *fs)
{
    _fs = fs;
    return FR_OK;
}

FRESULT df_maximize_file_size() { return FR_OK; }
