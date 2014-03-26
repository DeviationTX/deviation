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

/*The following will force the loading of various
  functions used in the protocol modules, but unused elsewhere
  in Deviation.
  Note that we lie aboiut the arguments to these functions. It is
  Important that the actual functions never execute
*/
#include "common.h"
#include "config/tx.h"
#include "protocol/interface.h"
#include <stdlib.h>
#include "protospi.h"

u32 AVR_StartProgram()
{
    u32 sync = 0;
    SPI_AVRProgramInit();
    for(int i = 0; i < 5; i++) {
        PROTOSPI_pin_set(AVR_RESET_PIN);
        usleep(10000);
        PROTOSPI_pin_clear(AVR_RESET_PIN);
        usleep(30000);
        PROTOSPI_xfer(0xAC);
        PROTOSPI_xfer(0x53);
        sync = PROTOSPI_xfer(0x00);
        PROTOSPI_xfer(0x00);
        if (sync == 0x53) {
            for(int j = 0; j < 3; j++) {
                PROTOSPI_xfer(0x30);
                PROTOSPI_xfer(0x00);
                PROTOSPI_xfer(j);
                sync |= PROTOSPI_xfer(0x00) << (8 * (j+1));
            }
            return sync;
        }
    }
    return 0;
}

int AVR_Erase()
{
    PROTOSPI_xfer(0xAC);
    PROTOSPI_xfer(0x80);
    PROTOSPI_xfer(0x00);
    PROTOSPI_xfer(0x00);
    usleep(20000);
    //verify 1st page is erased
    int ok = 1;
    for(int i = 0; i < 64; i++) {
        int pos = i/2;
        PROTOSPI_xfer(0x20 | ((i % 2) * 0x08));
        PROTOSPI_xfer(pos >> 8);
        PROTOSPI_xfer(pos & 0xff);
        u8 chk = PROTOSPI_xfer(0x00);
        if (chk != 0xff)
            ok = 0;
    }
    return ok;
}

int AVR_Program(u32 address, u8 *data, int pagesize)
{
    for(int i = 0; i < pagesize; i++) {
        PROTOSPI_xfer(0x40 | ((i % 2) * 0x08));
        PROTOSPI_xfer((i / 2) >> 8);
        PROTOSPI_xfer((i / 2) & 0xff);
        PROTOSPI_xfer(data[i]);
    }
    //Write
    PROTOSPI_xfer(0x4C);
    PROTOSPI_xfer(address >> 8);
    PROTOSPI_xfer(address & 0xff);
    PROTOSPI_xfer(0x00);
    usleep(4500);
    for(int i = 0; i < pagesize; i++) {
        int pos = address + i/2;
        PROTOSPI_xfer(0x20 | ((i % 2) * 0x08));
        PROTOSPI_xfer(pos >> 8);
        PROTOSPI_xfer(pos & 0xff);
        u8 chk = PROTOSPI_xfer(0x00);
        if (chk != data[i]) {
            printf("@%04x.%d: %02x != %02x\n", pos, i %2, chk, data[i]);
            return 0;
        }
    }
    return 1;
}

int AVR_Verify(u8 *data, int size)
{
    for(int i = 0; i < size; i++) {
        int pos = i/2;
        PROTOSPI_xfer(0x20 | ((i % 2) * 0x08));
        PROTOSPI_xfer(pos >> 8);
        PROTOSPI_xfer(pos & 0xff);
        u8 chk = PROTOSPI_xfer(0x00);
        if (chk != data[i]) {
            printf("@%04x.%d: %02x != %02x\n", pos, i %2, chk, data[i]);
            return 0;
        }
    }
    return 1;
}

int AVR_ResetFuses()
{
    int data;
    PROTOSPI_xfer(0xAC);
    PROTOSPI_xfer(0xA0);
    PROTOSPI_xfer(0x00);
    PROTOSPI_xfer(0x62); //enable divclkby8
    usleep(4500);
    PROTOSPI_xfer(0x50);
    PROTOSPI_xfer(0x00);
    PROTOSPI_xfer(0x00);
    data = PROTOSPI_xfer(0x00); //current fuse bits
    if (data != 0x62)
        return 0;
    return 1;
}

int AVR_SetFuses()
{
    int data;
    PROTOSPI_xfer(0x50);
    PROTOSPI_xfer(0x00);
    PROTOSPI_xfer(0x00);
    data = PROTOSPI_xfer(0x00); //current fuse bits

    if (data == 0xe2)
        return 1; //already programmed
    if (data != 0x62)
        return 0; //Fuse bits aren't properly set

    PROTOSPI_xfer(0xAC);
    PROTOSPI_xfer(0xA0);
    PROTOSPI_xfer(0x00);
    PROTOSPI_xfer(0xe2); //disable divclkby8
    usleep(4500);
    PROTOSPI_xfer(0x50);
    PROTOSPI_xfer(0x00);
    PROTOSPI_xfer(0x00);
    data = PROTOSPI_xfer(0x00); //current fuse bits
    if (data != 0xe2)
        return 0;
    return 1;
}

int AVR_VerifyFuses()
{
    PROTOSPI_xfer(0x50);
    PROTOSPI_xfer(0x00);
    PROTOSPI_xfer(0x00);
    int data = PROTOSPI_xfer(0x00); //current fuse bits
    return (data == 0xe2);
}
