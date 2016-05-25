#include "common.h"
#include "config/tx.h"

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
