#include "common.h"

void TxName(u8 *var, int len)
{
    strlcpy((char *)var, "X9D", len);
}

void MCU_SerialNumber(u8 *var, int len)
{
    // Every STM32 should have 12 bytes long unique id at 0x1FFFF7E8
    const u8 *stm32id = (u8*) 0x1FFFF7E8;
    int l = len > 12 ? 12 : len;
    for(int i = 0; i < l; i++) {
        var[i] = *stm32id++;
    }
    while(l < len) {
        var[l++] = 0x00;
    }
}

void _usleep(u32 x)
{
    /* Tuned for 48MHz Cortex-M0 */
    /* will be a little slow at values < 100 */
    (void)x;
    asm(".syntax unified");
    asm ("movs r1, #6;"
         "muls r1, r0, r1;"
         "lsrs r0, r0, #5;"
         "adds r0, r0, r1;"
         "b _delaycmp;"
         "_delayloop:"
         "subs r0, r0, #1;"
         "_delaycmp:;"
         "cmp r0, #0;"
         " bne _delayloop;");
    asm(".syntax divided");
}

//The following code came from: http://notabs.org/winzipcrc/winzipcrc.c
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
