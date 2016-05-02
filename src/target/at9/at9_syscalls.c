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


int _write_r (FATFS *r, char * ptr, int len)
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
    }
    errno=EINVAL;
    return -1;
}

