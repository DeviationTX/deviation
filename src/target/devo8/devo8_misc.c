#include "target.h"

u8* BOOTLOADER_Read(int idx) {
    u32 ret = 0x00000000;
    switch(idx) {
        case BL_ID: ret = 0x08001000; break;
    }
    return (u8*)ret;
}

void ModelName(u8 *var, u8 len)
{
    const u8 * pBLString = (u8*)0x08001000;
    if(len > 8)
        len = 8;
    memcpy(var, pBLString, len - 1);
    var[len - 1] = 0;
}

