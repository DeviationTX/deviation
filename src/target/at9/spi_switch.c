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

#if defined HAS_4IN1_FLASH && HAS_4IN1_FLASH

#define CS_HI() gpio_set(GPIOB, GPIO11)
#define CS_LO() gpio_clear(GPIOB, GPIO11)

// Numbers correspond to connection order on 4-in-1 board
// and on Discrete Logic board
enum {
   MODULE_A7105 = 0,
   MODULE_CC2500,
   MODULE_NRF24L01,
   MODULE_CYRF6936,
   MODULE_FLASH
};

// Traditional RF module definitions from target.h do not match 4-in-1
// module order
static int module_map[] = {
    MODULE_CYRF6936, MODULE_A7105, MODULE_CC2500, MODULE_NRF24L01
};


#define CYRF6936_RESET 0x40
#define NRF24L01_CE    0x80
static int last_module_used;
static u8 extra_bits;

void SPISwitch_Init()
{
    last_module_used = -1;
    extra_bits = NRF24L01_CE;
    /* CS for switch */
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO11);
    CS_HI();
}


static void UseModule(int module)
{
    if (module == last_module_used) return;

    u8 cmd = (module & 0x07) | extra_bits;
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

void SPISwitch_CS_HI(int module)
{
    (void) module;
    gpio_set(GPIOB, GPIO12);
}

void SPISwitch_CS_LO(int module)
{
    UseModule(module_map[module]);
    gpio_clear(GPIOB, GPIO12);
}
    

void SPISwitch_UseFlashModule()
{
    UseModule(MODULE_FLASH);
}


static void SetExtraBits(u8 mask, int state)
{
//    if (!!(extra_bits & mask) == state) return;
    if (state) extra_bits |= mask;
    else       extra_bits &= ~mask;
    u8 cmd = (last_module_used & 0x07) | extra_bits;
    CS_LO();
    spi_xfer(SPI2, cmd);
    CS_HI();
}

void SPISwitch_CYRF6936_RESET(int state)
{
    last_module_used = MODULE_CYRF6936;
    SetExtraBits(CYRF6936_RESET, state);
}

void SPISwitch_NRF24L01_CE(int state)
{
    last_module_used = MODULE_NRF24L01;
    SetExtraBits(NRF24L01_CE, state);
}

#endif // defined HAS_4IN1_FLASH && HAS_4IN1_FLASH
