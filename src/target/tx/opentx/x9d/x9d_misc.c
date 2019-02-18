#include "common.h"
void TxName(u8 *var, int len)
{
    strncpy((char *)var, "X9D", len);
}

void _usleep(u32 x)
{
    (void)x;
    asm ("mov r1, #20;"
         "mul r0, r0, r1;"
         "b _delaycmp;"
         "_delayloop:"
         "subs r0, r0, #1;"
         "_delaycmp:;"
         "cmp r0, #0;"
         " bne _delayloop;");
}
