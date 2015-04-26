#include "common.h"
#include "config/tx.h"

u8* BOOTLOADER_Read(int idx) {
    u32 ret = 0x00000000;
    switch(idx) {
        case BL_ID: ret = 0x08001000; break;
    }
    return (u8*)ret;
}

void TxName(u8 *var, int len)
{
    const u8 * pBLString = (u8*)0x08001000;
    if(len > 8)
        len = 8;
    memcpy(var, pBLString, len - 1);
    var[len - 1] = 0;
}

void MCU_SerialNumber(u8 *var, int len)
{
    if(Transmitter.txid) {
        int l = len > 16 ? 16 : len;
        u32 id[4];
        u32 seed = 0x4d3ab5d0ul;
        for(int i = 0; i < 4; i++)
            rand32_r(&seed, Transmitter.txid >> (8*i));
        for(int i = 0; i < 4; i++)
            id[i] = rand32_r(&seed, 0x00);
        memcpy(var, &id[0], l);
        return;
    }
    int l = len > 12 ? 12 : len;
    // Every STM32 should have 12 bytes long unique id at 0x1FFFF7E8
    const u8 *stm32id = (u8*) 0x1FFFF7E8;
    for(int i = 0; i < l; i++) {
        var[i] = *stm32id++;
    }
    while(l < len) {
        var[l++] = 0x00;
    }
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
