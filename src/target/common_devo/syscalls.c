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
#include <libopencm3/stm32/usart.h>

#include "petit_fat.h"
#include "common.h"

//#define dbgprintf(args...) printf(args)
#define dbgprintf(args...) 
static DIR   dir;
#ifdef MEDIA_DRIVE
static FATFS fat[2];
#else
static FATFS fat[1];
#endif

extern u8 _drive_num;

extern void init_err_handler();

int FS_Mount();
void FS_Unmount();

long _open_r (FATFS *r, const char *file, int flags, int mode);
int _close_r (FATFS *r);


int _read_r (FATFS *r, char * ptr, int len);
int _write_r (FATFS *r, char * ptr, int len);
int _lseek_r (FATFS *r, int ptr, int dir);


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
            pf_switchfile(&fat[0]);
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
    int res = pf_mount(f);
    f->pad1 = _drive_num;
    dbgprintf("Mount: %d\n", res);
    _drive_num = 0;
    return (res == FR_OK);
}

void FS_Unmount()
{
    pf_mount(0);
}

int FS_OpenDir(const char *path)
{
    FATFS *ptr = &fat[0];
#ifdef MEDIA_DRIVE
    if (strncmp(path, "media", 5) == 0) {
        ptr = &fat[1];
    }
#endif
    pf_switchfile(ptr);
    _drive_num = ptr->pad1;
    FRESULT res = pf_opendir(&dir, path);
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
    if (pf_readdir(&dir, &fi) != FR_OK || ! fi.fname[0])
        return 0;
    dbgprintf("Read: %s %d\n", fi.fname, fi.fattrib);
    strncpy(path, fi.fname, 13);
    return fi.fattrib & AM_DIR ? 2 : 1;
}

void FS_CloseDir()
{
}

long _open_r (FATFS *r, const char *file, int flags, int mode) {
    (void)flags;
    (void)mode;

    if(!r) {
#ifdef MEDIA_DRIVE
        if (strncmp(file, "media/", 6) == 0)
            r = &fat[1];
        else
#endif
            r = &fat[0];
    }
    if(r->flag & FA_OPENED) {
        dbgprintf("_open_r(%p): file already open.\n", r);
        return -1;
    } else {
        pf_switchfile(r);
        _drive_num = r->pad1;
        int res=pf_open(file);
        if(res==FR_OK) {
            dbgprintf("_open_r(%08lx): pf_open (%s) flags: %d, mode: %d ok\r\n", r, file, flags, mode);
            if (flags & O_CREAT)
                pf_maximize_file_size();
            return (long)r;
        } else {
            dbgprintf("_open_r(%08lx): pf_open (%s) failed: %d\r\n", r, file, res);
            return -1;
        }
    }
}

int _close_r (FATFS *r) {
    if(r) {
       r->flag = 0;
       dbgprintf("_close_r(%p): file closed.\r\n", r);
    }
    return 0;
}

int _read_r (FATFS *r, char * ptr, int len)
{
    if((unsigned long)r>2 && r->flag & FA_OPENED) {
        if(len <= 0xffff) {
            WORD bytes_read;
            pf_switchfile(r);
            _drive_num = r->pad1;
            int res=pf_read(ptr, len, &bytes_read);
            dbgprintf("_read_r: len %d, bytes_read %d, result %d\r\n", len, bytes_read, res); 
            if(res==FR_OK) return bytes_read;
        }
    }    

    errno=EINVAL;
    return -1;
}

int _write_r (FATFS *r, char * ptr, int len)
{  
    if((unsigned long)r==1 || (unsigned long)r==2) {
        int index;

        if (0 == (USART_CR1(USART1) & USART_CR1_UE))
            return len; //Don't send if USART is disabled

        for(index=0; index<len; index++) {
            if (ptr[index] == '\n') {
                usart_send_blocking(USART1,'\r');
            }  
            usart_send_blocking(USART1, ptr[index]);
        }    
        return len;
    } else if(r) {
        if(r->flag & FA_OPENED) {
            WORD bytes_written;
            pf_switchfile(r);
            _drive_num = r->pad1;
            int res=pf_write(ptr, len, &bytes_written);
            dbgprintf("_write_r: len %d, bytes_written %d, result %d\r\n",len, bytes_written, res);
            if(res==FR_OK) return bytes_written;
        }
    }
    errno=EINVAL;
    return -1;
}

long _ltell_r (FATFS *r)
{
    if((unsigned long)r>2 && (r->flag & FA_OPENED)) {
        return r->fptr;
    }
    return -1;
}

int _lseek_r (FATFS *r, int ptr, int dir)
{
    (void)r;
    
    if((unsigned long)r>2 && (r->flag & FA_OPENED)) {
        if(dir==SEEK_CUR) {
            ptr += r->fptr;
        } else if (dir==SEEK_END) {
            ptr += r->fsize;
        }
        pf_switchfile(r);
        _drive_num = r->pad1;
        int res=pf_lseek(ptr);
        if(res==FR_OK) {
           return r->fptr;
        }
    }
    errno=EINVAL;
    return -1;
}
