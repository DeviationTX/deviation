#include "target.h"

u8* BOOTLOADER_Read(int idx) {
    u32 ret = 0x00000000;
    switch(idx) {
        case BL_ID: ret = 0x08001000; break;
    }
    return (u8*)ret;
}
