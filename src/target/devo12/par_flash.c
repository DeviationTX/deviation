/*
    This project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Deviation is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/fsmc.h>
#include "common.h"

#define ADDRESS  0x64000000
#define OFFSET 0x80000

void PARFlash_Init()
{
    /* Extended mode, write enable, 16 bit access, bank enabled */
    FSMC_BCR2 = FSMC_BCR_MWID | FSMC_BCR_WREN | FSMC_BCR_MBKEN;

    /* Read & write timings */
    FSMC_BTR2  = FSMC_BTR_DATASTx(5) | FSMC_BTR_ADDHLDx(0) | FSMC_BTR_ADDSETx(2) | FSMC_BTR_ACCMODx(FSMC_BTx_ACCMOD_B);
    FSMC_BWTR2 = FSMC_BTR_DATASTx(5) | FSMC_BTR_ADDHLDx(0) | FSMC_BTR_ADDSETx(2) | FSMC_BTR_ACCMODx(FSMC_BTx_ACCMOD_B);
}

void PARFlash_ReadBytes(u32 readAddress, u32 length, u8 * buffer)
{
    u16 *ptr = (u16 *)(ADDRESS + OFFSET + readAddress);
    u16 *buf = (u16 *)buffer;
    while(length > 1) {
        *buf++ = *ptr++;
        length-=2;
    }
    if (length) {
        *(u8 *)buf = *ptr;
    }
}

int PARFlash_ReadBytesStopCR(u32 readAddress, u32 length, u8 * buffer)
{
    u32 i;
    u8 *ptr = (u8 *)(ADDRESS + OFFSET + readAddress);

    for(i=0;i<length;i++)
    {
        buffer[i] = *ptr++;
        if (buffer[i] == '\n') {
            i++;
            break;
        }
    }
    return i;
}
