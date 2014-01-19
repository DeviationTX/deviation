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
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/spi.h>
#include "common.h"
#include "devo.h"
#include "config/tx.h"
#include "protocol/interface.h"
#include <stdlib.h>

u32 AVR_StartProgram()
{
    const struct mcu_pin reset_pin = {.port = GPIO_BANK_JTCK_SWCLK, .pin = GPIO_JTCK_SWCLK} ;
    u32 sync = 0;
    spi_disable(SPI2);
    spi_set_baudrate_prescaler(SPI2, SPI_CR1_BR_FPCLK_DIV_256);
    spi_enable(SPI2);
    for(int i = 0; i < 5; i++) {
        gpio_set(reset_pin.port, reset_pin.pin);
        usleep(10000);
        gpio_clear(reset_pin.port, reset_pin.pin);
        usleep(30000);
        spi_xfer(SPI2, 0xAC);
        spi_xfer(SPI2, 0x53);
        sync = spi_xfer(SPI2, 0x00);
        spi_xfer(SPI2, 0x00);
        if (sync == 0x53) {
            for(int j = 0; j < 3; j++) {
                spi_xfer(SPI2, 0x30);
                spi_xfer(SPI2, 0x00);
                spi_xfer(SPI2, j);
                sync |= spi_xfer(SPI2, 0x00) << (8 * (j+1));
            }
            return sync;
        }
    }
    return 0;
}

int AVR_Erase()
{
    spi_xfer(SPI2, 0xAC);
    spi_xfer(SPI2, 0x80);
    spi_xfer(SPI2, 0x00);
    spi_xfer(SPI2, 0x00);
    usleep(20000);
    //verify 1st page is erased
    int ok = 1;
    for(int i = 0; i < 64; i++) {
        int pos = i/2;
        spi_xfer(SPI2, 0x20 | ((i % 2) * 0x08));
        spi_xfer(SPI2, pos >> 8);
        spi_xfer(SPI2, pos & 0xff);
        u8 chk = spi_xfer(SPI2, 0x00);
        if (chk != 0xff)
            ok = 0;
    }
    return ok;
}

int AVR_Program(u32 address, u8 *data, int pagesize)
{
    for(int i = 0; i < pagesize; i++) {
        spi_xfer(SPI2, 0x40 | ((i % 2) * 0x08));
        spi_xfer(SPI2, (i / 2) >> 8);
        spi_xfer(SPI2, (i / 2) & 0xff);
        spi_xfer(SPI2, data[i]);
    }
    //Write
    spi_xfer(SPI2, 0x4C);
    spi_xfer(SPI2, address >> 8);
    spi_xfer(SPI2, address & 0xff);
    spi_xfer(SPI2, 0x00);
    usleep(4500);
    for(int i = 0; i < pagesize; i++) {
        int pos = address + i/2;
        spi_xfer(SPI2, 0x20 | ((i % 2) * 0x08));
        spi_xfer(SPI2, pos >> 8);
        spi_xfer(SPI2, pos & 0xff);
        u8 chk = spi_xfer(SPI2, 0x00);
        if (chk != data[i]) {
            printf("@%04x.%d: %02x != %02x\n", pos, i %2, chk, data[i]);
            return 0;
        }
    }
    return 1;
}

int AVR_SetFuses()
{
    int data;
    spi_xfer(SPI2, 0x50);
    spi_xfer(SPI2, 0x00);
    spi_xfer(SPI2, 0x00);
    data = spi_xfer(SPI2, 0x00); //current fuse bits
    if (data == 0xe2)
        return 1; //already programmed
    if (data != 0x62)
        return 0; //Fuse bits aren't properly set
    spi_xfer(SPI2, 0xAC);
    spi_xfer(SPI2, 0xA0);
    spi_xfer(SPI2, 0x00);
    spi_xfer(SPI2, 0xe2); //disable divclkby8
    usleep(4500);
    spi_xfer(SPI2, 0x50);
    spi_xfer(SPI2, 0x00);
    spi_xfer(SPI2, 0x00);
    data = spi_xfer(SPI2, 0x00); //current fuse bits
    if (data != 0xe2)
        return 0;
    return 1;
}
