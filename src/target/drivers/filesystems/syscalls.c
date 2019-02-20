/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "common.h"

#if ! defined(EMULATOR) || EMULATOR == USE_INTERNAL_FS
extern void printstr(char *, int);
//#define dbgprintf(args...) printf(args)
#define dbgprintf(args...) 
static DIR   dir;
#ifdef MEDIA_DRIVE
static FATFS fat[2];
#else
static FATFS fat[1];
#endif

#ifdef FIL  // If FIL is #defined, it maps to FATFS
    #define fil fat
#else
    static FIL fil[1];
#endif
extern u8 _drive_num;

extern void init_err_handler();

int FS_Mount();
void FS_Unmount();

intptr_t _open_r(FIL *r, const char *file, int flags, int mode);
int _close_r(FIL *r);


int _read_r(FIL *r, char * ptr, int len);
int _write_r(FIL *r, char * ptr, int len);
int _lseek_r(FIL *r, int ptr, int dir);


int FS_Mount(void *_f, const char *drive)
{
    (void)drive;
    if(! _f) {
#ifdef MEDIA_DRIVE
        int res = (FS_Mount(&fat[0], "")  && FS_Mount(&fat[1], "media"));
#else 
        int res = FS_Mount(&fat[0], "");
#endif
        if (res) {
            init_err_handler();
            fs_switchfile(&fat[0]);
            return 1;
        }
        return 0;
    }
    FATFS *f = (FATFS *)_f;
    _drive_num = 0;
#ifdef MEDIA_DRIVE
    if (strncmp(drive, "media", 5) == 0) {
        _drive_num = 1;
    }
#endif
    int res = fs_mount(f);
    fs_set_drive_num(f, _drive_num);
    dbgprintf("Mount: %d\n", res);
    _drive_num = 0;
    return (res == FR_OK);
}

void FS_Unmount()
{
    fs_mount(0);
}

int FS_OpenDir(const char *path)
{
    FATFS *ptr = &fat[0];
#ifdef MEDIA_DRIVE
    if (strncmp(path, "media", 5) == 0) {
        ptr = &fat[1];
    }
#endif
    fs_switchfile(ptr);
    _drive_num = fs_get_drive_num(ptr);
    FRESULT res = fs_opendir(&dir, path);
    dbgprintf("Opendir: %d\n", res);
    return (res == FR_OK);
}

/* Return:
   0 : Error
   1 : File
   2 : Dir
*/
int FS_ReadDir(char *path)
{
    FILINFO fi;
    int res = fs_readdir(&dir, &fi);
    dbgprintf("ReadDir: %d\n", res);
    if (res != FR_OK || !fi.fname[0])
        return 0;
    dbgprintf("Read: %s %d\n", fi.fname, fi.fattrib);
    strlcpy(path, fi.fname, 13);
    return fi.fattrib & AM_DIR ? 2 : 1;
}

void FS_CloseDir()
{
}

intptr_t _open_r(FIL *r, const char *file, int flags, int mode) {
    (void)flags;
    (void)mode;

    if(!r) {
#ifdef MEDIA_DRIVE
        if (strncmp(file, "media/", 6) == 0)
            r = &fil[1];
        else
#endif
            r = &fil[0];
    }
    if(fs_is_open(r)) {
        dbgprintf("_open_r(%p): file already open.\n", r);
        return -1;
    } else {
        fs_switchfile(r);
        _drive_num = fs_get_drive_num(r);
        int res = fs_open(r, file, flags, mode);
        if (res == FR_OK) {
            dbgprintf("_open_r(%08lx): fs_open (%s) flags: %d, mode: %d ok\r\n", r, file, flags, mode);
            if (flags & O_CREAT)
                fs_maximize_file_size();
            return (long)r;
        } else {
            dbgprintf("_open_r(%08lx): fs_open (%s) failed: %d\r\n", r, file, res);
            return -1;
        }
    }
}

int _close_r(FIL *r) {
    if(r) {
       fs_close(r);
       dbgprintf("_close_r(%p): file closed.\r\n", r);
    }
    return 0;
}

int _read_r(FIL *r, char * ptr, int len)
{
    if((unsigned long)r>2 && fs_is_open(r)) {
        if(len <= 0xffff) {
            unsigned  bytes_read = 0;
            fs_switchfile(r);
            _drive_num = fs_get_drive_num(r);
            int res = fs_read(r, ptr, len, &bytes_read);
            dbgprintf("_read_r: len %d, bytes_read %d, result %d\r\n", len, bytes_read, res); 
            if (res == FR_OK) return bytes_read;
        }
    }    

    errno = EINVAL;
    return -1;
}

int _write_r(FIL *r, char * ptr, int len)
{  
    if((unsigned long)r==1 || (unsigned long)r==2) {
        printstr(ptr, len);
        return len;
    } else if(r) {
        if(fs_is_open(r)) {
            unsigned bytes_written = 0;
            fs_switchfile(r);
            _drive_num = fs_get_drive_num(r);
            int res = fs_write(r, ptr, len, &bytes_written);
            dbgprintf("_write_r: len %d, bytes_written %d, result %d\r\n",len, bytes_written, res);
            if (res == FR_OK) return bytes_written;
        }
    }
    errno = EINVAL;
    return -1;
}

int _ltell_r(FIL *r)
{
    if ((intptr_t)r > 2 && fs_is_open(r)) {
        return (int)fs_ltell(r);
    }
    return -1;
}

int _lseek_r(FIL *r, int ptr, int dir)
{
    (void)r;
    
    if ((intptr_t)r > 2 && fs_is_open(r)) {
        if (dir == SEEK_CUR) {
            ptr += fs_ltell(r);
        } else if (dir == SEEK_END) {
            ptr += fs_filesize(r);
        }
        fs_switchfile(r);
        _drive_num = fs_get_drive_num(r);
        int res = fs_lseek(r, ptr);
        if (res == FR_OK) {
           return fs_ltell(r);
        }
    }
    errno = EINVAL;
    return -1;
}
#endif  // !defined(EMULATOR) || EMULATOR == USE_INTERNAL_FS
