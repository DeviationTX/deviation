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
#define FIL void

extern void init_err_handler();

long _open_r (FIL *r, const char *file, int flags, int mode);
int _close_r (FIL *r);


int _read_r (FIL *r, char * ptr, int len);
int _write_r (FIL *r, char * ptr, int len);
int _lseek_r (FIL *r, int ptr, int dir);


int _close_r (FIL *r) {
    (void) r;
    return 0;
}

int _read_r (FIL *r, char * ptr, int len)
{
    (void) r;
    (void) ptr;
    (void) len;
    errno=EINVAL;
    return -1;
}

int _write_r (FIL *r, char * ptr, int len)
{  
        (void) r;
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

long _ltell_r (FIL *r)
{
    (void) r;
    return -1;
}

int _lseek_r (FIL *r, int ptr, int dir)
{
    (void) r;
    (void) ptr;
    (void) dir;
    errno=EINVAL;
    return -1;
}
