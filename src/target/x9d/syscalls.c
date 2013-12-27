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

#include "common.h"

//#define dbgprintf(args...) printf(args)
#define dbgprintf(args...) 
static DIR   dir;
static FATFS fat[1];
static FIL fil;

extern void init_err_handler();

int FS_Mount();
void FS_Unmount();

long _open_r (FIL *r, const char *file, int flags, int mode);
int _close_r (FIL *r);


int _read_r (FIL *r, char * ptr, int len);
int _write_r (FIL *r, char * ptr, int len);
int _lseek_r (FIL *r, int ptr, int dir);


int FS_Mount(void *_f, const char *drive)
{
    (void)drive;
    FATFS *f = (FATFS *)_f;
    if (! f)
        f = &fat[0];
    int res = f_mount(0, f);
    dbgprintf("Mount: %d\n", res);
    return (res == FR_OK);
}

void FS_Unmount()
{
    f_mount(0, NULL);
}

int FS_OpenDir(const char *path)
{
    FRESULT res = f_opendir(&dir, path);
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
    if (f_readdir(&dir, &fi) != FR_OK || ! fi.fname[0])
        return 0;
    dbgprintf("Read: %s %d\n", fi.fname, fi.fattrib);
    strncpy(path, fi.fname, 13);
    return fi.fattrib & AM_DIR ? 2 : 1;
}

void FS_CloseDir()
{
}

long _open_r (FIL *r, const char *file, int flags, int mode) {
    (void)flags;
    (void)mode;

    if(!r) {
            r = &fil;
    }
    if(1) {
	int fa;
        if (mode == O_RDONLY)
            fa = FA_READ;
        else {
            fa = FA_WRITE;
            if (flags & O_CREAT)
                fa |= FA_CREATE_ALWAYS;
        }
        int res=f_open(r, file, fa);
        if(res==FR_OK) {
            dbgprintf("_open_r(%08lx): pf_open (%s) flags: %d, mode: %d ok\r\n", r, file, flags, mode);
            return (long)r;
        } else {
            dbgprintf("_open_r(%08lx): pf_open (%s) failed: %d\r\n", r, file, res);
            return -1;
        }
    }
}

int _close_r (FIL *r) {
    if(r) {
       f_close(r);
       dbgprintf("_close_r(%p): file closed.\r\n", r);
    }
    return 0;
}

int _read_r (FIL *r, char * ptr, int len)
{
    if((unsigned long)r>2) {
        if(len <= 0xffff) {
            UINT bytes_read;
            int res=f_read(r, ptr, len, &bytes_read);
            dbgprintf("_read_r: len %d, bytes_read %d, result %d\r\n", len, bytes_read, res); 
            if(res==FR_OK) return bytes_read;
        }
    }    

    errno=EINVAL;
    return -1;
}

int _write_r (FIL *r, char * ptr, int len)
{  
    if((unsigned long)r==1 || (unsigned long)r==2) {
        int index;

        if (0 == (USART_CR1(_USART) & USART_CR1_UE))
            return len; //Don't send if USART is disabled

        for(index=0; index<len; index++) {
            if (ptr[index] == '\n') {
                usart_send_blocking(_USART,'\r');
            }  
            usart_send_blocking(_USART, ptr[index]);
        }    
        return len;
    } else if(r) {
        UINT bytes_written;
        int res=f_write(r, ptr, len, &bytes_written);
        dbgprintf("_write_r: len %d, bytes_written %d, result %d\r\n",len, bytes_written, res);
        if(res==FR_OK) return bytes_written;
    }
    errno=EINVAL;
    return -1;
}

long _ltell_r (FIL *r)
{
    if((unsigned long)r>2) {
        return r->fptr;
    }
    return -1;
}

int _lseek_r (FIL *r, int ptr, int dir)
{
    (void)r;
    
    if((unsigned long)r>2) {
        if(dir==SEEK_CUR) {
            ptr += r->fptr;
        } else if (dir==SEEK_END) {
            ptr += r->fsize;
        }
        int res=f_lseek(r, ptr);
        if(res==FR_OK) {
           return r->fptr;
        }
    }
    errno=EINVAL;
    return -1;
}
