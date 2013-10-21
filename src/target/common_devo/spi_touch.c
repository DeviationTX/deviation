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
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/spi.h>
#include "common.h"
#include "config/tx.h"
#include "devo.h"

#define START   (1 << 7)     // 1 = user command, 0 = reserved for factory purposes

#define ADR_Y   (1 << 4)    // address of y register
#define ADR_RST (2 << 4)    // address of control register
#define ADR_Z1  (3 << 4)    // address of z register (pressure)
#define ADR_Z2  (4 << 4)    // address of 2nd z register (pressure)
#define ADR_X   (5 << 4)    // address of x register

#define MODE    (0 << 3)     // for next conversion: 0 = 12bit, 1 = 8bit (see datasheet tables 2+3)
#define SER     (1 << 2)     // 1 = single ended, 0 = differential
#define PD      (0 << 0)     // PowerDown Mode: 0 = power down between conversions (see datasheet table 5)

// different settings for control register
#define CONTROL (2 << 4)
#define PULLUP  (0 << 3)    // PENIRQ pullup resistor: 0 = 50kOhm, 1 = 90kOhm
// enabling MAV
#define BYPASS  (0 << 2)    // MAV filter: 0 = enabled, 1 = bypassed (returns the average of the best 3 of 7 measurings)
#define TIMING  (0 << 1)    // Timing (for SPI bus, SDO)
#define NORESET (0 << 0)    // software reset: 1 = reset, 0 = nothing happens
#define SWRESET (1 << 0)    // software reset: 1 = reset, 0 = nothing happens

#define READ_X  START | ADR_X   | MODE | SER | PD
#define READ_Y  START | ADR_Y   | MODE | SER | PD
#define READ_Z1 START | ADR_Z1  | MODE | SER | PD
#define READ_Z2 START | ADR_Z2  | MODE | SER | PD
// different for control command:
   #define SET     START | CONTROL | PULLUP | BYPASS | TIMING | NORESET
   #define RESET   SET | SWRESET

/*
PB0 : Chip Select
PB5 : PenIRQ
PA5 : SPI1_SCK
PA6 : SPI1_MISO
PA7 : SPI1_MOSI
*/

#define CS_HI() gpio_set(_TOUCH_PORT, _TOUCH_PIN)
#define CS_LO() gpio_clear(_TOUCH_PORT, _TOUCH_PIN)
#define pen_is_down() (! gpio_get(_TOUCH_PORT, _TOUCH_IRQ_PIN))

u8 read_channel(u8 address)
{
    spi_xfer(SPI1, address);
    while(pen_is_down())
        ;
    return spi_xfer(SPI1, 0x00);
}

void SPITouch_Init()
{
    if(! HAS_TOUCH)
        return;
#if 0
    /* Enable SPI1 */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_SPI1EN);
    /* Enable GPIOA */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);
    /* Enable GPIOB */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);
#endif
    /* CS */
    gpio_set_mode(_TOUCH_PORT, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, _TOUCH_PIN);

    /* PenIRQ is pull-up input*/
    gpio_set_mode(_TOUCH_PORT, GPIO_MODE_INPUT,
                  GPIO_CNF_INPUT_PULL_UPDOWN, _TOUCH_IRQ_PIN);
    gpio_set(_TOUCH_PORT, _TOUCH_IRQ_PIN);

    CS_LO();
    spi_xfer(SPI1, RESET);
    CS_HI();
    //SPITouch_Calibrate(197312, 147271, -404, -20); /* Read from my Tx */
#if 0
    /* SCK, MOSI */
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO5 | GPIO7);
    /* MISO */
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
                  GPIO_CNF_INPUT_FLOAT, GPIO6);
    /* Includes enable */
    spi_init_master(SPI1, 
                    SPI_CR1_BAUDRATE_FPCLK_DIV_4,
                    SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_2, 
                    SPI_CR1_DFF_8BIT,
                    SPI_CR1_MSBFIRST);
    spi_enable_software_slave_management(SPI1);
    spi_set_nss_high(SPI1);

    spi_enable(SPI1);
#endif
}

struct touch SPITouch_GetCoords()
{
    #define TOUCH_READS 3
    struct touch res, data[TOUCH_READS];
    u32 center_x = 0, center_y = 0;
    CS_LO();
    // read TOUCH_READS times from the touchpad and store the values
    for (int i=0; i<TOUCH_READS; i++) {
        #if _TOUCH_COORDS_REVERSE
            /* X and Y are swapped on Devo8 */
            /* and X is reversed */
            data[i].x = 255 - read_channel(READ_Y);
            data[i].y = read_channel(READ_X);
        #else
            data[i].x = 255 - read_channel(READ_X);
            data[i].y = read_channel(READ_Y);
        #endif
        // TODO: is it necessary to read all values? Must they also be calculated (or: are they used somewhere? AFAIK only in tx_configure.c for display testing)?
        data[i].z1 = read_channel(READ_Z1);
        data[i].z2 = read_channel(READ_Z2);
        // add values to calculate center of measuring points
        center_x += data[i].x;
        center_y += data[i].y;
    }
    CS_HI();
    #if TOUCH_READS > 1
        // divide by number of measurements to correct the range
        center_x /= TOUCH_READS;
        center_y /= TOUCH_READS;
        // search for the data point with the greatest distance to the calculated center
        u32 dist = 0, max_dist = 0;
        int index_max_dist = 0;
        for (int i=0; i<TOUCH_READS; i++) {
            dist = ((center_x - data[i].x) * (center_x - data[i].x) + (center_y - data[i].y) * (center_y - data[i].y));
            // rather dist^2, but 'dist > 0' and 'a > b ==> a^2 > b^2' this will suffice to search the outlier
            if (dist > max_dist) {
                max_dist = dist;
                index_max_dist = i;
            }
        }
        // reset this data point
        data[index_max_dist].x = 0;
        data[index_max_dist].y = 0;
        // recalculate the center
        center_x = 0;
        center_y = 0;
        for (int i=0; i<TOUCH_READS; i++) {
            center_x += data[i].x;
            center_y += data[i].y;
        }
        // divide by number of measurements -1 (!, one data point discarded) to correct the range and store
        res.x = center_x / (TOUCH_READS - 1);
        res.y = center_y / (TOUCH_READS - 1);
    #else
        res.x = data[0].x;
        res_y = data[0].y;
    #endif
    res.x = res.x * Transmitter.touch.xscale / 0x10000 + Transmitter.touch.xoffset;
    if(res.x & 0x8000)
        res.x = 0;
    res.y = res.y * Transmitter.touch.yscale / 0x10000 + Transmitter.touch.yoffset;
    if(res.y & 0x8000)
        res.y = 0;
    return res;
}

int SPITouch_IRQ()
{
    return pen_is_down();
}

void SPITouch_Calibrate(s32 xscale, s32 yscale, s32 xoff, s32 yoff)
{
    Transmitter.touch.xscale = xscale;
    Transmitter.touch.yscale = yscale;
    Transmitter.touch.xoffset = xoff;
    Transmitter.touch.yoffset = yoff;
}
