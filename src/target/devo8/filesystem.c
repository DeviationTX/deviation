/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Foobar is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "petit_fat.h"
#include <stdio.h>

static FATFS fat;

#define DBGFS if(0) printf

void FS_Mount()
{
    int res = pf_mount(&fat);
    DBGFS("Mount: %d\n", res);
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

FILE *fopen(const char *path, const char *mode)
{
    (void)mode;
    int res = pf_open(path);
    DBGFS("fopen %s: %d\n", path, res);
    return res ? 0 : (FILE *)&fat;
}

int fclose(FILE *fp)
{
    (void)fp;
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

