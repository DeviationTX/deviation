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

#include "petit_fat.h"
#include <stdio.h>

static FATFS fat;

#ifdef SHOW_LOADTIME
  #include "target.h"
  #include <libopencm3/stm32/usart.h>
  static u32 ms;
  static char f[80];
#endif
#define DBGFS if(0) printf

int FS_Mount()
{
    int res = pf_mount(&fat);
    DBGFS("Mount: %d\n", res);
    return (res == FR_OK);
}

void FS_Unmount()
{
    pf_mount(0);
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    int res;
    WORD len;
    size_t bytes = size * nmemb;

    (void)stream;
    if(bytes >= 0x10000) {
        printf("Size %d is bigger than allowed read 0xFFFF\n", bytes);
        return 0;
    }
    res = pf_read(ptr, (WORD)bytes, &len);
    DBGFS("fread %d: req: %d got: %d\n", res, bytes, len);
    return len / size;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    (void)stream;
    WORD len;
    int res;

    res = pf_write(ptr, size * nmemb, &len);
    DBGFS("fwrite %d: req: %d got: %d\n", res, size * nmemb, len);
    return (len == size * nmemb) ? nmemb : 0;
}

FILE *fopen(const char *path, const char *mode)
{
    (void)mode;
#ifdef SHOW_LOADTIME
    strcpy(f, path);
    ms = CLOCK_getms();
#endif
    int res = pf_open(path);
    if (mode[0] == 'w')
        pf_maximize_file_size(); //When writing a file, make sure it takes up the full sector
    DBGFS("fopen %s: %d\n", path, res);
    return res ? 0 : (FILE *)&fat;
}

int fclose(FILE *fp)
{
    (void)fp;
#ifdef SHOW_LOADTIME
/*  The system hangs if we try to do a printf on the filename,
    or modify the ordering of the below in any way,
    but the below code DOES work if enabled */
    u32 t = CLOCK_getms();

    printf("fclose: %d", (int)(t - ms));
    usart_send_blocking(USART1, ' ');
    usart_send_blocking(USART1, f[0]);
    usart_send_blocking(USART1, f[1]);
    usart_send_blocking(USART1, f[2]);
    usart_send_blocking(USART1, f[3]);
    usart_send_blocking(USART1, f[4]);
    usart_send_blocking(USART1, f[5]);
    usart_send_blocking(USART1, f[6]);
    usart_send_blocking(USART1, f[7]);
    usart_send_blocking(USART1, f[8]);
    usart_send_blocking(USART1, f[9]);
    usart_send_blocking(USART1, f[10]);
    usart_send_blocking(USART1, f[11]);
    usart_send_blocking(USART1, f[12]);
    usart_send_blocking(USART1, f[13]);
    usart_send_blocking(USART1, '\n');
    usart_send_blocking(USART1, '\r');
#endif
    return 0;
}

int fseek(FILE *stream, long offset, int whence) {
    (void)stream;
    if(whence == SEEK_CUR)
        offset += fat.fptr;
    else if(whence == SEEK_END)
        offset += fat.fsize;
    int res = pf_lseek(offset);
    DBGFS("fseek(%d): %d, %d -> %d\n", (int)res, (int)offset, (int)whence, (int)fat.fptr);
    return res;
}

