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
#include <libopencm3/stm32/usart.h>

#include "petit_fat.h"

//#define dbgprintf(args...) printf(args)
#define dbgprintf(args...)

static FATFS fat;
static bool file_open=false;

int FS_Mount();
void FS_Unmount();

int _open_r (struct _reent *r, const char *file, int flags, int mode);
int _close_r (struct _reent *r, int fd);


int _read_r (struct _reent *r, int fd, char * ptr, int len);
int _write_r (struct _reent *r, int fd, char * ptr, int len);
int _lseek_r (struct _reent *r, int fd, int ptr, int dir);


caddr_t _sbrk_r (struct _reent *r, int incr);
int _fstat_r (struct _reent *r, int fd, struct stat * st);
int _isatty_r(struct _reent *r, int fd);

int FS_Mount()
{
    int res = pf_mount(&fat);
    dbgprintf("Mount: %d\n", res);
    return (res == FR_OK);
}

void FS_Unmount()
{
    pf_mount(0);
}

int _open_r (struct _reent *r, const char *file, int flags, int mode) {
    (void)r;
    (void)flags;
    (void)mode;

    if(file_open) {
        dbgprintf("_open_r: file already open.\n");
        return -1;
    } else {
        int res=pf_open(file);
        if(res==FR_OK) {
            dbgprintf("_open_r: pf_open (%s) ok\r\n", file);
            file_open=true;
            return 3;  
        } else {
            dbgprintf("_open_r: pf_open failed\r\n");
            return -1;
        }
    }
}

int _close_r (struct _reent *r, int fd) {
    (void)r;
    if(fd==3) {
       file_open=false;
       dbgprintf("_close_r: file closed.\r\n");
    }
    return 0;
}

int _read_r (struct _reent *r, int fd, char * ptr, int len)
{
    (void)r;
    if(fd==3 && file_open) {
        if(len <= 0xffff) {
            WORD bytes_read;
            int res=pf_read(ptr, len, &bytes_read);
            dbgprintf("_read_r: len %d, bytes_read %d, result %d\r\n", len, bytes_read, res); 
            if(res==FR_OK) return bytes_read;
        }
    }    

    errno=EINVAL;
    return -1;
}

int _write_r (struct _reent *r, int fd, char * ptr, int len)
{  
    (void)r;
    if(fd==1 || fd==2) {
        int index;
  
        for(index=0; index<len; index++) {
            if (ptr[index] == '\n') {
                usart_send_blocking(USART1,'\r');
            }  
            usart_send_blocking(USART1, ptr[index]);
        }    
        return len;
    } else if(fd==3) {
        if(file_open) {
            WORD bytes_written;
            int res=pf_write(ptr, len, &bytes_written);
            dbgprintf("_write_r: len %d, bytes_written %d, result %d\r\n",len, bytes_written, res);
            if(res==FR_OK) return bytes_written;
        }
    }
    errno=EINVAL;
    return -1;
}


int _lseek_r (struct _reent *r, int fd, int ptr, int dir)
{
    (void)r;
    
    if(fd==3 && file_open) {
        if(dir==SEEK_CUR) {
            ptr += fat.fptr;
        } else if (dir==SEEK_END) {
            ptr += fat.fsize;
        }
        int res=pf_lseek(ptr);
        if(res==FR_OK) {
           return fat.fptr;
        }
    }
    errno=EINVAL;
    return -1;
}



int _fstat_r (struct _reent *r, int fd, struct stat * st)
{
    (void)r;
    (void)fd;
    (void)st;
    errno=EINVAL;
    return -1;
}

int _isatty_r(struct _reent *r, int fd)
{
    (void)r;
    if(fd<3) return 1;
    return 0;  
}



register char * stack_ptr asm ("sp");

caddr_t _sbrk_r (struct _reent *r, int incr)
{
  extern char   end asm ("end"); /* Defined by the linker.  */
  static char * heap_end;
  char *        prev_heap_end;

   r=r;

  if (heap_end == NULL)
    heap_end = & end;
  
  prev_heap_end = heap_end;
  
  if (heap_end + incr > stack_ptr)
  {
    errno = ENOMEM;
    return (caddr_t) -1;
  }
  
  heap_end += incr;

  return (caddr_t) prev_heap_end;
}
