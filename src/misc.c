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
#include <limits.h>

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
    const char str1 = 0x3f & *(str + 1);
    const char str2 = 0x3f & *(str + 2);
    const char str3 = 0x3f & *(str + 3);
    const char str4 = 0x3f & *(str + 4);
    const char str5 = 0x3f & *(str + 5);
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

//strlcpy from
//http://stackoverflow.com/questions/1453876/why-does-strncpy-not-null-terminate
size_t strlcpy(char* dst, const char* src, size_t bufsize)
{
  size_t srclen =strlen(src);
  size_t result =srclen; /* Result is always the length of the src string */
  if(bufsize>0)
  {
    if(srclen>=bufsize)
       srclen=bufsize-1;
    if(srclen>0)
       memcpy(dst,src,srclen);
    dst[srclen]='\0';
  }
  return result;
}

void tempstring_cpy(const char* src)
{
    strlcpy(tempstring, src, sizeof(tempstring));
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

static u32 rand_seed = 0xb2c54a2ful;
// Linear feedback shift register with 32-bit Xilinx polinomial x^32 + x^22 + x^2 + x + 1
static const uint32_t LFSR_FEEDBACK = 0x80200003ul;
static const uint32_t LFSR_INTAP = 32-1;
static void update_lfsr(uint32_t *lfsr, uint8_t b)
{
    for (int i = 0; i < 8; ++i) {
        *lfsr = (*lfsr >> 1) ^ ((-(*lfsr & 1u) & LFSR_FEEDBACK) ^ ~((uint32_t)(b & 1) << LFSR_INTAP));
        b >>= 1;
    }
}
u32 rand32_r(u32 *seed, u8 update)
{
    if(! seed)
        seed = &rand_seed;
    update_lfsr(seed, update);
    return *seed;
}
u32 rand32()
{
    return rand32_r(0, 0);
}

int fexists(const char *file)
{
   FILE *fh = fopen(file, "r");
   if(! fh)
       return 0;
   fclose(fh);
   return 1;
}

/* USB */
void wait_press_release(u32 press)
{
    printf("Wait %s\n", press ? "Press" : "Release");
    while(1) {
        CLOCK_ResetWatchdog();
        u32 buttons = ScanButtons();
        if (CHAN_ButtonIsPressed(buttons, BUT_ENTER) == press)
            break;
        if(PWR_CheckPowerSwitch())
            PWR_Shutdown();
    }
    printf("%sed\n", press ? "Press" : "Release");
}

void wait_press() {
    wait_press_release(CHAN_ButtonMask(BUT_ENTER));
}
void wait_release()
{
    wait_press_release(0);
}

void USB_Connect()
{
    USB_Enable(0, 1);
    //Disable USB Exit
    while(1) {
        if(PWR_CheckPowerSwitch())
            PWR_Shutdown();
    }
    wait_release();
    wait_press();
    wait_release();
    USB_Disable();
}

