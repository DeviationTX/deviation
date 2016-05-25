#include "common.h"
#include "config/tx.h"

void _usleep(u32 x)
{
    (void)x;
    asm ("mov r1, #24;"
         "mul r0, r0, r1;"
         "b _delaycmp;"
         "_delayloop:"
         "subs r0, r0, #1;"
         "_delaycmp:;"
         "cmp r0, #0;"
         " bne _delayloop;");
}
