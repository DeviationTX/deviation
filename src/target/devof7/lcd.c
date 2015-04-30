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
#include "lcd.h"

u32 IA9211_map_char(u32 c);

static const struct font_def default_font = {1, 1};
static const struct font_def big_font = {2, 2};
struct font_str cur_str;
static u16 is_double_row = 0x00;

#define CUR_CHAR_SIZE  (cur_str.font.height)

#define CS_HI() gpio_set(GPIOB, GPIO0)
#define CS_LO() gpio_clear(GPIOB, GPIO0)

// The screen dimensions
#define LCD_SCREEN_LINES    11
#define LCD_SCREEN_CHARS    24

// The IA911 chip defines
#define LCD_IA911_CLEAR_VRAM        0x00
#define LCD_IA911_DISPLAY           0x10
#define LCD_IA911_COLOR             0x20
#define LCD_IA911_BACKGROUND        0x30
#define LCD_IA911_CLOCK             0x40
#define LCD_IA911_VIDEO_SEL         0x48
#define LCD_IA911_OSSIL_SEL         0x50
#define LCD_IA911_DISPLAY_POS       0x8000
#define LCD_IA911_WRITE_ADDR        0x8800
#define LCD_IA911_OUTPUT_LVL        0x9004
#define LCD_IA911_CHAR_SIZE         0x9800
#define LCD_IA911_TEST_MODE         0xB000
#define LCD_IA911_DISPLAY_CHAR      0xC0

// IA911 Display options
#define LCD_IA911_DO                (0x1<<3)
#define LCD_IA911_LC                (0x1<<2)
#define LCD_IA911_BL1               (0x1<<1)
#define LCD_IA911_BL0               (0x1<<0)

// IA911 Color options
#define LCD_IA911_R                 (0x1<<3)
#define LCD_IA911_G                 (0x1<<2)
#define LCD_IA911_B                 (0x1<<1)

// IA911 Background options
#define LCD_IA911_BS1               (0x1<<2)
#define LCD_IA911_BS0               (0x1<<1)

// IA911 Clock options
#define LCD_IA911_I_MODE            (0x1<<2)
#define LCD_IA911_XOSC              (0x1<<0)

// IA911 Video select options
#define LCD_IA911_P2                (0x1<<2)
#define LCD_IA911_P1                (0x1<<1)
#define LCD_IA911_P0                (0x1<<0)

// IA911 Ossilation mode options
#define LCD_IA911_XFC               (0x1<<1)

// IA911 character size options
#define LCD_IA911_S0                (0x1<<6)

// IA911 Output level options
#define LCD_IA911_VPD               (0x1<<8)
#define LCD_IA911_VC1               (0x1<<1)
#define LCD_IA911_VC0               (0x1<<0)

// IA911 Display char options
#define LCD_IA911_BL                (0x1<<1)
#define LCD_IA911_DISPLAY_OFF       0x7E
#define LCD_IA911_TRANSFER_END      0x7F


void LCD_CMDLength(const u8 cmd[], u8 length) {
    volatile int i, j;
    CS_LO();

    // Wait a couple of clock ticks
    i = 100;
    while(i) i--;

    for(i = 0; i < length; i++) {
        spi_xfer(SPI1, cmd[i]);

        // Wait a couple of clock ticks
        j = 100;
        while(j) j--;
    }
    CS_HI();

    // Wait a couple of clock ticks
    i = 100;
    while(i) i--;
}

void LCD_Cmd(const u16 cmd) {
    u8 new_cmd[2];
    new_cmd[0] = cmd >> 8;
    new_cmd[1] = cmd & 0xFF;

    if(cmd>>8)
        LCD_CMDLength(new_cmd, 2);
    else
        LCD_CMDLength(&new_cmd[1], 1);
}

void LCD_Contrast(unsigned contrast)
{
    (void) contrast;
}

// Convert a string with the charmap
void lcd_convert_string(const char string[], u8 length, u8* output) {
    u8 i;
    for(i = 0; i < length; i++)
        output[i] = IA9211_map_char(string[i]);
}

// Calculate string length
u8 lcd_string_length(const char string[]) {
    unsigned i = 0;
    unsigned size = 0;
    while(string[i] != 0 && size < LCD_SCREEN_CHARS) {
         i++;
         size += CUR_CHAR_SIZE; // chwaracter width
    }
    return i;
}

// Show a string at a certain position
void lcd_show_string(const char string[], u8 line, s8 pos, u16 color) {
    u8 cmd[LCD_SCREEN_CHARS+2];
    u8 length = lcd_string_length(string);
    if(pos < 0)
        pos = LCD_SCREEN_CHARS-length*CUR_CHAR_SIZE+pos+1;

    // Check if it fits inside the screen
    if(line > LCD_SCREEN_LINES || pos > LCD_SCREEN_CHARS)
        return;

    //fix line due to big-font rows
    //NOTE:  This will ony work properly if screen is drawn from top to bottom.
    //       Otherwise adding a double row at the top will cause the bottom items to shift
    unsigned double_line = 0;
    for(unsigned i = 0; i < line; i++) {
        if (is_double_row & ( 1 << i)) {
           double_line++;
        }
    }

    if ((is_double_row & ( 1 << line)) && CUR_CHAR_SIZE == 1) {
        is_double_row  &= ~( 1 << line);
        LCD_Cmd(LCD_IA911_CHAR_SIZE | (line - double_line));
    } else if (! (is_double_row & ( 1 << line)) && CUR_CHAR_SIZE == 2) {
        is_double_row  |= ( 1 << line);
        LCD_Cmd(LCD_IA911_CHAR_SIZE | 0x40 | (line - double_line));
    }
    if (pos+length * CUR_CHAR_SIZE > LCD_SCREEN_CHARS) {
        length = (LCD_SCREEN_CHARS - pos) >> (CUR_CHAR_SIZE == 1 ? 0 : 1);
    }

    line -= double_line;

    // Send the position
    LCD_Cmd(LCD_IA911_WRITE_ADDR | ((line << 5) + pos));

    // Start sending characters
    cmd[0] = LCD_IA911_DISPLAY_CHAR;
    if(color != 0xFFFF)
        cmd[0] = cmd[0] | LCD_IA911_BL; //Blink

    // Convert the string
    lcd_convert_string(string, length, &cmd[1]);

    // Send the string
    cmd[length+1] = LCD_IA911_TRANSFER_END;
    LCD_CMDLength(cmd, length+2);
}

// Show a string at a certain line
void lcd_show_line(const char string[], u8 line, u8 align, u16 color) {
    char new_string[LCD_SCREEN_CHARS];
    u8 pos_x, i, j;
    u8 length = lcd_string_length(string);

    // Check if it is inside the screen
    if(line > LCD_SCREEN_LINES || length > LCD_SCREEN_CHARS)
        return;

    // Calculate the X position
    if(align == LCD_ALIGN_LEFT)
        pos_x = 0;
    else if(align == LCD_ALIGN_CENTER)
        pos_x = (LCD_SCREEN_CHARS - length) / 2;
    else
        pos_x = (LCD_SCREEN_CHARS - length);

    // Create the new string
    j = 0;
    for(i = 0; i < LCD_SCREEN_CHARS; i++) {
        if(i < pos_x || pos_x+length <= i)
            new_string[i] = ' '; //del
        else {
            new_string[i] = string[j];
            j++;
        }
    }

    lcd_show_string(new_string, line, 0, color);
}

void LCD_Init()
{
    /* Enable GPIOA clock. */
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPAEN);

    /* Set GPIO0, GPIO8, GPIO15, GPIO4 (in GPIO port A) to 'output push-pull'. */
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
		  GPIO_CNF_OUTPUT_PUSHPULL, GPIO0);  //CS0
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
		  GPIO_CNF_OUTPUT_PUSHPULL, GPIO8);  //CS1
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ,
		  GPIO_CNF_OUTPUT_PUSHPULL, GPIO15); //CS2
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ,
		  GPIO_CNF_OUTPUT_PUSHPULL, GPIO9);  //ON, OFF?

    // Set the 5.8GHz receiver off (default)
    gpio_clear(GPIOB, GPIO9);

    // Set the 5.8GHz channel
    gpio_set(GPIOA, GPIO0);
    gpio_set(GPIOA, GPIO8);
    gpio_set(GPIOA, GPIO15);

    //Initialization is mostly done in SPI Flash
    //Setup CS as B.0 Data/Control = C.5
    rcc_peripheral_enable_clock(&RCC_APB2ENR, RCC_APB2ENR_IOPBEN);
    gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
                  GPIO_CNF_OUTPUT_PUSHPULL, GPIO0);

    LCD_Cmd(LCD_IA911_CLEAR_VRAM); //Clear the VRAM

    // Wait for a couple of clock ticks
    volatile int i = 800;
    while(i) i--;

    LCD_Cmd(LCD_IA911_DISPLAY | LCD_IA911_DO | LCD_IA911_LC | LCD_IA911_BL1); // Display on, LC ossillator on, Blink at 1Hz
    LCD_Cmd(LCD_IA911_COLOR | LCD_IA911_B); // Background color blue
    LCD_Cmd(LCD_IA911_BACKGROUND | LCD_IA911_BS0); // Background black framing
    LCD_Cmd(LCD_IA911_VIDEO_SEL | LCD_IA911_P0); // Set video to PAL
    LCD_Cmd(LCD_IA911_OSSIL_SEL | LCD_IA911_XFC); // Set ossilator XOSCI pin connected
    LCD_Cmd(LCD_IA911_OUTPUT_LVL | LCD_IA911_VC1 | LCD_IA911_VC0); // Set screen brightness to 90 IRE
    LCD_Cmd(LCD_IA911_TEST_MODE); // Test the internal circuit
    LCD_Cmd(LCD_IA911_CLOCK | LCD_IA911_I_MODE | LCD_IA911_XOSC); // Set to internal mode en enable ossilator (0x45)
    LCD_Cmd(LCD_IA911_DISPLAY_POS | (13-1)<<4 | (5-1)); //13 Vertical and 5 Horizontal as display position

    // Set the character size to 1dot for all lines
    for(i = 0; i < LCD_SCREEN_LINES; i++)
        LCD_Cmd(LCD_IA911_CHAR_SIZE | i);
}

void LCD_Clear(unsigned int val)
{
    u8 i;
    (void) val;

    for(i = 0; i <= LCD_SCREEN_LINES; i++)
        lcd_show_line("", i, LCD_ALIGN_LEFT, 0);
}

void LCD_DrawStart(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, enum DrawDir _dir)
{
    (void) x0; (void) y0; (void) x1; (void) y1; (void) _dir;
}

void LCD_DrawStop(void)
{

}

void LCD_DrawPixel(unsigned int color)
{
   (void) color;
}

void LCD_DrawPixelXY(unsigned int x, unsigned int y, unsigned int color)
{
    (void) x; (void) y; (void) color;
}

u8 video_enabled = 0;
void LCD_ShowVideo(u8 enable)
{
    if(enable == video_enabled)
        return;

    if(enable) {
        gpio_set(GPIOB, GPIO9);
        LCD_Cmd(LCD_IA911_CLOCK | LCD_IA911_XOSC); // Set to external mode en enable ossilator
    }
    else {
        gpio_clear(GPIOB, GPIO9);
        LCD_Cmd(LCD_IA911_CLOCK | LCD_IA911_I_MODE | LCD_IA911_XOSC); // Set to external mode en enable ossilator
    }

    video_enabled = enable;
}


/**
 * Since this is a text based screen some of the LCD string functions are here
 **/
u8 FONT_GetFromString(const char *value)
{
    if (strcmp(value, "big") == 0) {
        return 2;
    }
    return 1;
}

void LCD_PrintCharXY(unsigned int x, unsigned int y, u32 c)
{
    LCD_ShowVideo(0);
    lcd_show_string((char*)&c, y, x, cur_str.color);
}

u8 LCD_SetFont(unsigned int idx)
{
    u8 old = LCD_GetFont();
    cur_str.font = (idx <= 1) ? default_font : big_font;
    return old;
}
