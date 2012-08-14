#include "misc.h"

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
u32 Crc(void *buffer, u32 size)
{
   u32 crc = ~0;
   u8  *position = buffer;

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

