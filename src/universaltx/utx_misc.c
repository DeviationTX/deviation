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
