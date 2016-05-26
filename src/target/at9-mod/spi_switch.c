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
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include "common.h"

#define CS_HI() gpio_set(GPIOB, GPIO11)
#define CS_LO() gpio_clear(GPIOB, GPIO11)


static int last_module_used;
void SPISwitch_Init()
{
    last_module_used = -1;
    /* CS for switch */
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO11);
    CS_HI();
}


void SPISwitch_UseModule(int module)
{
    if (module == last_module_used) return;

    u8 cmd = (module & 0x07) | 0xC0;
    CS_LO();
    spi_xfer(SPI2, cmd);
    CS_HI();

    // Adjust baud rate
    spi_disable(SPI2);
    if (module == MODULE_FLASH) {
        spi_set_baudrate_prescaler(SPI2, SPI_CR1_BR_FPCLK_DIV_4);
    } else {
        spi_set_baudrate_prescaler(SPI2, SPI_CR1_BR_FPCLK_DIV_16);
    }
    spi_enable(SPI2);

    last_module_used = module;
}
