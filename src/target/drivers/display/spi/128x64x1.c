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
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>
#include "common.h"
#include "gui/gui.h"
#include "target/drivers/mcu/stm32/rcc.h"

#define CS_HI() GPIO_pin_set(LCD_SPI.csn)
#define CS_LO() GPIO_pin_clear(LCD_SPI.csn)
#define CMD_MODE() GPIO_pin_clear(LCD_SPI_MODE)
#define DATA_MODE() GPIO_pin_set(LCD_SPI_MODE)

#ifndef OLED_SPI_RATE
    #define OLED_SPI_RATE 0
#endif

static void LCD_Cmd(unsigned cmd) {
    CMD_MODE();
    CS_LO();
    spi_xfer(LCD_SPI.spi, cmd);
    CS_HI();
}

static void LCD_Data(unsigned cmd) {
    DATA_MODE();
    CS_LO();
    spi_xfer(LCD_SPI.spi, cmd);
    CS_HI();
}

static void lcd_init_ports()
{
    // Initialization is mostly done in SPI Flash
    // Setup CS as B.0 Data/Control = C.5
    rcc_periph_clock_enable(get_rcc_from_pin(LCD_SPI.csn));
    rcc_periph_clock_enable(get_rcc_from_pin(LCD_SPI_MODE));
    GPIO_setup_output(LCD_SPI.csn, OTYPE_PUSHPULL);
    GPIO_setup_output(LCD_SPI_MODE, OTYPE_PUSHPULL);
    if (HAS_OLED_DISPLAY)
        spi_set_baudrate_prescaler(LCD_SPI.spi, OLED_SPI_RATE);
}

#include "128x64x1_common.h"
