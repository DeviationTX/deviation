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
#include <libopencm3/stm32/spi.h>
#include "common.h"

#if defined HAS_4IN1_FLASH && HAS_4IN1_FLASH
#include "target/drivers/mcu/stm32/rcc.h"

#define CS_HI() GPIO_pin_set(PROTO_RST_PIN)
#define CS_LO() GPIO_pin_clear(PROTO_RST_PIN)

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
static unsigned switch_detected, flash_present;

static void UseModule(int module);

/* Find out, do we have SPI switch board installed */
static void detect()
{
    switch_detected = 0;
    u32 res;
    unsigned cyrf_present = 0;
    flash_present = 0;
    
    printf("Detecting SPI Switch\n");
    /* Check that CYRF6936 is present */
    /* CYRF6936 reset */
    CS_HI();
    Delay(100);
    CS_LO();
    Delay(100);
    GPIO_pin_clear(PROTO_SPI.csn);
    spi_xfer(PROTO_SPI.spi, 0x10);
    res = spi_xfer(PROTO_SPI.spi, 0);
    GPIO_pin_set(PROTO_SPI.csn);
    if (res == 0xa5) {
        printf("CYRF6936 directly available, no switch\n");
        return;
    }
    CS_HI();
    /* Switch to CYRF6936 module */
    UseModule(MODULE_CYRF6936);
    /* Check that CYRF6936 is present */
    SPISwitch_CYRF6936_RESET(1);
    Delay(100);
    SPISwitch_CYRF6936_RESET(0);
    Delay(100);
    GPIO_pin_clear(PROTO_SPI.csn);
    spi_xfer(PROTO_SPI.spi, 0x10);
    res = spi_xfer(PROTO_SPI.spi, 0);
    GPIO_pin_set(PROTO_SPI.csn);
    if (res == 0xa5) {
        printf("CYRF6936 found at pos 3\n");
        cyrf_present = 1;
    }
    /* Switch to flash module */
    UseModule(MODULE_FLASH);
    /* Check that JEDEC ID command returns non-zero and not all bits high */
    GPIO_pin_clear(PROTO_SPI.csn);
    spi_xfer(PROTO_SPI.spi, 0x9F);
    res  = (u8)spi_xfer(PROTO_SPI.spi, 0);
    res <<= 8;
    res |= (u8)spi_xfer(PROTO_SPI.spi, 0);
    res <<= 8;
    res |= (u8)spi_xfer(PROTO_SPI.spi, 0);
    GPIO_pin_set(PROTO_SPI.csn);
    if (res && res != 0xFFFFFF) {
        printf("Flash detected, JEDEC ID: '%X'\n", res);
        flash_present = 1;
    }
    switch_detected = cyrf_present || flash_present;
    printf("SPI Switch %s\n", switch_detected ? "detected" : "absent");
}

void SPISwitch_Init()
{
    last_module_used = -1;
    extra_bits = NRF24L01_CE;
    rcc_periph_clock_enable(get_rcc_from_pin(PROTO_RST_PIN));
    /* RST used as CS for switch */
    GPIO_setup_output(PROTO_RST_PIN, OTYPE_PUSHPULL);
    detect();
}

unsigned SPISwitch_Present()
{
    return switch_detected;
}

unsigned SPISwitch_FlashPresent()
{
    return flash_present;
}

static void UseModule(int module)
{
    if (module == last_module_used) return;

    u8 cmd = (module & 0x07) | extra_bits;
    CS_LO();
    spi_xfer(PROTO_SPI.spi, cmd);
    CS_HI();

    // Adjust baud rate
    spi_disable(PROTO_SPI.spi);
    if (module == MODULE_FLASH) {
        spi_set_baudrate_prescaler(PROTO_SPI.spi, SPI_CR1_BR_FPCLK_DIV_4);
    } else if (last_module_used == MODULE_FLASH) {
        spi_set_baudrate_prescaler(PROTO_SPI.spi, SPI_CR1_BR_FPCLK_DIV_16);
    }
    spi_enable(PROTO_SPI.spi);

    last_module_used = module;
}

void SPISwitch_CS_HI(int module)
{
    (void) module;
    GPIO_pin_set(PROTO_SPI.csn);
}

void SPISwitch_CS_LO(int module)
{
    UseModule(module_map[module]);
    GPIO_pin_clear(PROTO_SPI.csn);
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
    spi_xfer(PROTO_SPI.spi, cmd);
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
