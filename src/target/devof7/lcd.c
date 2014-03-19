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
#include <libopencm3/stm32/spi.h>
#include "common.h"
#include "gui/gui.h"
#include "lcd.h"

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


// The character mapping
static unsigned char charmap[] = {
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //0
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //8
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //16
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //24
    0x10, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //32
    0x7E, 0x7E, 0x7E, 0x7E, 0x0F, 0x0D, 0x0E, 0x6D, //40
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, //48
    0x08, 0x09, 0x0A, 0x67, 0x0B, 0x00, 0x0C, 0x50, //56
    0x00, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, //64
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x00, //72
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, //80
    0x28, 0x29, 0x2A, 0x00, 0x00, 0x00, 0x00, 0x00, //88
    0x00, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, //96
    0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, //104
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, //112
    0x68, 0x69, 0x6A, 0x00, 0x00, 0x00, 0x00, 0x7E, //120
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //128
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //136
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //144
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //152
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //160
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //168
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //176
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //184
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //192
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //200
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //208
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //216
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //224
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //232
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //240
    0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, //248
};

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

void LCD_Contrast(u8 contrast)
{
    (void) contrast;
}

// Convert a string with the charmap
void lcd_convert_string(const char string[], u8 length, u8* output) {
    u8 i;
    for(i = 0; i < length; i++)
        output[i] = charmap[(u8)string[i]];
}

// Calculate string length
u8 lcd_string_length(const char string[]) {
    u8 i = 0;
    while(string[i] != 0 && i < LCD_SCREEN_CHARS) i++;
    return i;
}

// Show a string at a certain position
void lcd_show_string(const char string[], u8 line, s8 pos, u16 color) {
    u8 cmd[LCD_SCREEN_CHARS+2];
    u8 length = lcd_string_length(string);
    if(pos < 0)
        pos = LCD_SCREEN_CHARS-length+pos+1;

    // Check if it fits inside the screen
    if(line > LCD_SCREEN_LINES || pos+length > LCD_SCREEN_CHARS)
        return;

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
            new_string[i] = 0x7F; //del
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

    // Set the 5.8GHz receiver on
    gpio_set(GPIOB, GPIO9);

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

    if(enable)
        LCD_Cmd(LCD_IA911_CLOCK | LCD_IA911_XOSC); // Set to external mode en enable ossilator
    else
        LCD_Cmd(LCD_IA911_CLOCK | LCD_IA911_I_MODE | LCD_IA911_XOSC); // Set to external mode en enable ossilator

    video_enabled = enable;
}


/**
 * Since this is a text based screen some of the LCD string functions are here
 **/
u8 FONT_GetFromString(const char *value)
{
    (void) value;
    return 0;
}

void LCD_PrintCharXY(unsigned int x, unsigned int y, u32 c)
{
    LCD_ShowVideo(0);
    lcd_show_string((char*)&c, y, x, cur_str.color);
}

u8 LCD_SetFont(unsigned int idx)
{
    (void) idx;
    return 0;
}

u8 LCD_GetFont()
{
    return 0;
}