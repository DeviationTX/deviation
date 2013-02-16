#include "common.h"

u8* BOOTLOADER_Read(int idx) {
    u32 ret = 0x00000000;
    switch(idx) {
        case BL_ID: ret = 0x08001000; break;
    }
    return (u8*)ret;
}

void TxName(u8 *var, u8 len)
{
    const u8 * pBLString = (u8*)0x08001000;
    if(len > 8)
        len = 8;
    memcpy(var, pBLString, len - 1);
    var[len - 1] = 0;
}

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
