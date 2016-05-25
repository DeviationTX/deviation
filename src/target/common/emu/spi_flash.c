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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "common.h"

#if EMULATOR == USE_INTERNAL_FS

static unsigned char *_data;
void SPIFlash_Init()
{
    int fd = open("devo.fs", O_RDWR);
    if (fd == -1) {
        printf("ERROR: Can't open devo.fs\n");
        _exit(1);
    }
    struct stat st;
    fstat(fd, &st);
    _data = mmap((caddr_t)0, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}

void SPIFlash_EraseSector(u32 sectorAddress)
{
    unsigned char *ptr = _data + sectorAddress;
    memset(ptr, 0, 4096);
}

void SPIFlash_WriteBytes(u32 writeAddress, u32 length, const unsigned char * buffer)
{
    unsigned char *ptr = _data + writeAddress;
    memcpy(ptr, buffer, length);
}

void SPIFlash_WriteByte(u32 writeAddress, const unsigned byte)
{
    unsigned char *ptr = _data + writeAddress;
    *ptr = byte;
}

void SPIFlash_ReadBytes(u32 readAddress, u32 length, unsigned char * buffer)
{
    unsigned char *ptr = _data + readAddress;
    memcpy(buffer, ptr, length);
}
int SPIFlash_ReadBytesStopCR(u32 readAddress, u32 length, unsigned char * buffer)
{
    u32 i;
    unsigned char *ptr = _data + readAddress;
    for(i = 0; i < length; i++) {
        buffer[i] = ptr[i];
        if (ptr[i] == '\n') {
            i++;
            break;
        }
    }
    return i;
}

#endif //EMULATOR == USE_INTERNAL_FS
