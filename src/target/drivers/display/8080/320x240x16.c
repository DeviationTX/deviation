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
#include "common.h"
#include "gui/gui.h"
#include "target/drivers/mcu/stm32/fsmc.h"
#include "320x240x16.h"

static u8 screen_flip;
static const struct lcdtype *disp_type;
static void lcd_cmd(uint8_t addr, uint8_t data);

#include "320x240x16_hx8347.h"
#include "320x240x16_ili9341.h"
#include "480x320x16_st7796.h"

static void lcd_cmd(uint8_t addr, uint8_t data)
{
    LCD_REG = addr;
    LCD_DATA = data;
}

static void lcd_set_pos(unsigned int x0, unsigned int y0)
{
    if (screen_flip)
        y0 = LCD_HEIGHT - 1 - y0;
    disp_type->set_pos(x0, y0);
    // printf("lcd_set_pos: %d, %d\n", x0, y0);
}

static u16 lcd_read_id() {
    u8 data;
    // Read ID register
    LCD_REG = 0xd3;
    // As per the spec, the 1st 2 reads are dummy reads and irrelevant
    data = LCD_DATA;
    data = LCD_DATA;
    // Actual ID is in 3rd and 4th bytes
    data = LCD_DATA;
    u16 data2 = LCD_DATA;
    data2 = (((int)data) << 8) | data2;

    return data2;
}

// LCDTYPE_XXX is a bitmask enum.
enum {
    LCDTYPE_UNKNOWN     = 0x00,
    LCDTYPE_HX8347      = 0x01,
    LCDTYPE_ILI9341     = 0x02,
    LCDTYPE_ST7796      = 0x04
};
#define HAS_LCD_TYPE(x) ((HAS_LCD_TYPES) & x)

int lcd_detect()
{
#ifdef LCD_RESET_PIN
    /* Reset pin for ILI9341 */
    GPIO_setup_output(LCD_RESET_PIN, OTYPE_PUSHPULL);
    GPIO_pin_clear(LCD_RESET_PIN);
    _usleep(10);   // must be held low for at least 10us
    GPIO_pin_set(LCD_RESET_PIN);
    _msleep(120);  // must wait 120ms after reset
#endif  // LCD_RESET_PIN

    if (HAS_LCD_TYPE(LCDTYPE_HX8347)) {
        // Read ID register for HX8347 (will be 0x47 if found)
        LCD_REG = 0x00;
        u8 data = LCD_DATA;
        if (data == 0x47) {
            hx8347_init();
            return LCDTYPE_HX8347;
        }
    }
    if (HAS_LCD_TYPE(LCDTYPE_ILI9341)) {
        if (lcd_read_id() == 0x9341) {
            ili9341_init();
            return LCDTYPE_ILI9341;
        }
    }
    if (HAS_LCD_TYPE(LCDTYPE_ST7796)) {
        if (lcd_read_id() == 0x7796) {
            st7796_init();
            return LCDTYPE_ST7796;
        }
    }

    printf("No LCD detected\n");
    return LCDTYPE_UNKNOWN;
}

void LCD_DrawPixel(unsigned int color)
{
    LCD_DATA = color;
}

void LCD_DrawPixelXY(unsigned int x, unsigned int y, unsigned int color)
{
    lcd_set_pos(x, y);
    LCD_DATA = color;
}

void LCD_DrawStart(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, enum DrawDir dir)
{
    if (dir == DRAW_SWNE) {
        unsigned int y = LCD_HEIGHT - 1 - y0;
        y0 = LCD_HEIGHT - 1 - y1;
        y1 = y;
        screen_flip = 1;
    } else {
        screen_flip = 0;
    }

    // printf("LCD_DrawStart: (%d, %d) - (%d, %d)\n", x0, y0, x1, y1);
    disp_type->draw_start(x0, y0, x1, y1);
    return;
}

void LCD_DrawStop(void)
{
    return;
}

void LCD_Sleep()
{
    disp_type->sleep();
}

void LCD_Init()
{
    _fsmc_init(
        16,
        0x10000 /*only bit 16 of addr */,
        FSMC_NOE | FSMC_NWE |FSMC_NE1,
        FSMC_BANK1,
        FSMC_BCR_MWID | FSMC_BCR_WREN | FSMC_BCR_MBKEN,
        FSMC_BTR_DATASTx(2) | FSMC_BTR_ADDHLDx(0) | FSMC_BTR_ADDSETx(1) | FSMC_BTR_ACCMODx(FSMC_BTx_ACCMOD_B),
        FSMC_BTR_DATASTx(2) | FSMC_BTR_ADDHLDx(0) | FSMC_BTR_ADDSETx(1) | FSMC_BTR_ACCMODx(FSMC_BTx_ACCMOD_B));

    while (lcd_detect() == LCDTYPE_UNKNOWN) {
        // retry inititalize and detect
    }
}

void LCD_Contrast(unsigned contrast)
{
    (void)contrast;  // dummy method for devo8. Only valid in devo10 now
}
