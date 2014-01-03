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

#include "common.h"
#include <stdlib.h>

void Delay(u32 count)
{
    while(count) {
        int i = 0; //72000;
        while(i) {
            i--;
            asm volatile ("nop"); // prevent the optimizer from removing this loop
        }
        count--;
    }
}

//The folloiwng code came from: http://notabs.org/winzipcrc/winzipcrc.c
// C99 winzip crc function, by Scott Duplichan
//We could use the internal CRC implementation in the STM32, but this is really small
//and perfomrance isn't really an issue
u32 Crc(const void *buffer, u32 size)
{
   u32 crc = ~0;
   const u8  *position = buffer;

   while (size--) 
      {
      int bit;
      crc ^= *position++;
      for (bit = 0; bit < 8; bit++) 
         {
         s32 out = crc & 1;
         crc >>= 1;
         crc ^= -out & 0xEDB88320;
         }
      }
   return ~crc;
}

/* Note that the following does no error checking on whether the string
 * is valid utf-8 or even if the length is ok.  Caveat Emptor.
 */
const char *utf8_to_u32(const char *str, u32 *ch)
{
    char str1 = 0x3f & *(str + 1);
    char str2 = 0x3f & *(str + 2);
    char str3 = 0x3f & *(str + 2);
    char str4 = 0x3f & *(str + 2);
    char str5 = 0x3f & *(str + 2);
    if(! (*str & 0x80)) {
        *ch = *str;
        return str + 1;
    }
    if((*str & 0xE0) == 0xC0) {
        /* 2 bytes */
        *ch = ((0x1F & *str) << 6) | str1;
        return str + 2;
    }
    if((*str & 0xF0) == 0xE0) {
        /* 3 bytes */
        *ch = ((0x0F & *str) << 12) | (str1 << 6) | str2;
        return str + 3;
    }
    if((*str & 0xF8) == 0xF0) {
        /* 4 bytes */
        *ch = ((0x07 & *str) << 18) | (str1 << 12) | (str2 << 6) | str3;
        return str + 4;
    }
    if((*str & 0xFC) == 0xF8) {
        /* 5 bytes */
        *ch = ((0x03 & *str) << 24) | (str1 << 18) | (str2 << 12) | (str3 << 6) | str4;
        return str + 5;
    }
    if((*str & 0xFE) == 0xFC) {
        /* 6 bytes */
        *ch = ((0x01 & *str) << 30) | (str1 << 24) | (str2 << 18) | (str3 << 12) | (str4 << 6) | str5;
        return str + 6;
    }
    return NULL;
}

int exact_atoi(const char *str)
{
    char *endptr;
    int value;
    value = strtol(str, &endptr, 10);
    if (*endptr != '\0')
        value = 0;
    return value;
}

int fexists(const char *file)
{
   FILE *fh = fopen(file, "r");
   if(! fh)
       return 0;
   fclose(fh);
   return 1;
}
