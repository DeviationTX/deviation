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

    This file is partially based upon the CooCox ILI9341S driver
    http://www.coocox.org/driver_repo/305488dd-734b-4cce-a8a4-39dcfef8cc66/html/group___i_l_i9341_s.html
*/

#include <libopencm3/stm32/gpio.h>
#include "common.h"
#include "lcd.h"

//Set this to the number of trials
#define DEBUG_SCREEN_ALIGNMENT 0
#include "tw8816_init_1.5.h"
//#include "tw8816_init_1.3.h"
//#include "tw8816_init_bl1.3a.h"
//#include "tw8816_init_1.5.h"

void wait_button() {
    u32 buttons = ScanButtons();
    while(! CHAN_ButtonIsPressed(buttons, BUT_EXIT))
        buttons = ScanButtons();
    _msleep(100);
    while( CHAN_ButtonIsPressed(buttons, BUT_EXIT))
        buttons = ScanButtons();
    _msleep(100);
}

extern u8 window;


void TW8816_Init()
{
#if DEBUG_SCREEN_ALIGNMENT
    u8 reg[sizeof(reg_init)];
    struct _map {
        u8 grp;
        u8 idx;
        u8 reg;
        u8 val;
    } map[] = {
      {99, 102, 0xB4, 0x10},
      {1, 102, 0xB4, 0x18},
      {2, 102, 0xB4, 0x20},
      {3, 102, 0xB4, 0x28},
      {4, 102, 0xB4, 0x2E},
      {5, 102, 0xB4, 0x2E},
      {5, 107, 0xB9, 0x18},
      {6, 102, 0xB4, 0x2E},
      {6, 107, 0xB9, 0x1A},
      {7, 102, 0xB4, 0x2E},
      {7, 107, 0xB9, 0x1C},
      {8, 102, 0xB4, 0x2E},
      {8, 107, 0xB9, 0x1E},
      {9, 102, 0xB4, 0x2E},
      {9, 107, 0xB9, 0x20},
    };
    BACKLIGHT_Init();
    BACKLIGHT_Brightness(1);
    for (int ii = 0; ii < DEBUG_SCREEN_ALIGNMENT ; ii++) {
        memcpy(reg, reg_init, sizeof(reg_init));
        for (unsigned i = 0; i < sizeof(map) / sizeof(struct _map); i++) {
            if(map[i].grp == ii) {
                reg[map[i].idx*2+1] = map[i].val;
            }
        }
#else
    const u8 *reg = reg_init;
    {
#endif
        TW8816_Reset();
        printf("1\n");
        for (unsigned i = 0; i < sizeof(reg_init); i+=2) {
            LCD_WriteReg(reg[i], reg[i+1]);
        }
        LCD_WriteReg(0xFF, 0x00);
        TW8816_SetVideoMode(0);
        TW8816_ReinitPixelClock();
        LCD_WriteReg(0xFF, 0x00);
        if (LCD_ReadReg(0x00) != 0x22) {
            printf("Could not identify display\n");
        }
        printf("3\n");
        //Clear screen
        LCD_WriteReg(0xFF, 0);
#if DEBUG_SCREEN_ALIGNMENT
        LCD_WriteReg(0x94, 0);
        //Setup normal display
        LCD_WriteReg(0x9e, 0x00);
        LCD_WriteReg(0x9f, 0x01);
        LCD_WriteReg(0xa0, 0x00);
        LCD_WriteReg(0xa1, 0x00);
        LCD_WriteReg(0xa2, 0x00);
        LCD_WriteReg(0xa6, 0x00);
        LCD_WriteReg(0xa7, 0x00);
        LCD_WriteReg(0xa3, 33);
        LCD_WriteReg(0xa4, 13);
        LCD_WriteReg(0xAC, 0x08);
        LCD_WriteReg(0xA9,  0x50);
        LCD_WriteReg(0x94, 2);
        TW8816_DisplayCharacter(0, 'A' + ii, 7);
        TW8816_DisplayCharacter(1, 'A' + ii, 7);
        TW8816_DisplayCharacter(31, 'A' + ii, 7);
        TW8816_DisplayCharacter(32, 'A' + ii, 7);
        TW8816_DisplayCharacter(12*33, 'A' + ii, 7);
        TW8816_DisplayCharacter(12*33+32, 'A' + ii, 7);
        wait_button();
#endif
    }
    //Setup XY Graph
    LCD_WriteReg(0x9e, 0x00);
    LCD_WriteReg(0x9f, 0x01);
    LCD_WriteReg(0xa0, (564 >> 8));
    LCD_WriteReg(0xa1, 0xff & 564); //564
    LCD_WriteReg(0xa2, 126); //126
    LCD_WriteReg(0xa3, 6);
    LCD_WriteReg(0xa4, 4);
    LCD_WriteReg(0xa5, 6);
    LCD_WriteReg(0xa6, 0x00);
    LCD_WriteReg(0xa7, 0x00);
    LCD_WriteReg(0xa8, 0x00);
    LCD_WriteReg(0xa9, 0xA1);
    LCD_WriteReg(0xaa, 0xAE); //0x1AE = 430
    LCD_WriteReg(0xAB,  0x12);
    LCD_WriteReg(0xAC,   0x08);
    LCD_WriteReg(0xAD,     0);
    LCD_WriteReg(0xAE,     0);
    window = 1;
    //for(int i = 0; i < 24; i++)
    //    TW8816_DisplayCharacter(i, 'A' + i, 7);

    //Setup normal display
    LCD_WriteReg(0x9e, 0x01);
    LCD_WriteReg(0x9f, 0x01);
    LCD_WriteReg(0xa0, 0x00);
    LCD_WriteReg(0xa1, 0x00);
    LCD_WriteReg(0xa2, 0x00);
    LCD_WriteReg(0xa6, 0x00);
    LCD_WriteReg(0xa7, 0x00);
    LCD_WriteReg(0xa3, 33);
    LCD_WriteReg(0xa4, 13);
    LCD_WriteReg(0xAC, 0x08);
    LCD_WriteReg(0xA9,  0x50);
    window = 0;
    //for(int i = 0; i < 24; i++)
    //    TW8816_DisplayCharacter(i, 'A' + i, 7);
}

void TW8816_Reset()
{
    gpio_clear(GPIOE, GPIO7);
    _msleep(250);
    gpio_set(GPIOE, GPIO7);
    _msleep(100);
}

void TW8816_ResetLoop()
{
    u8 count = 0;
    TW8816_Reset();
    while(1) {
        LCD_WriteReg(0xFF, 0x00);
        if (LCD_ReadReg(0x00) == 0x22)
            break;
        count++;
        if (count < 250) {
            Delay(0x800);
        } else {
            count = 0;
            TW8816_Reset();
        }
    }
}

void TW8816_LoadFont(u8 *data, unsigned offset, unsigned count)
{
    LCD_WriteReg(0x94, 1);
    LCD_WriteReg(0x9B, 0xE2);
    LCD_WriteReg(0xE0, 0x10);
    for (unsigned i = 0; i < count; i++) {
        LCD_WriteReg(0x99, offset + i);
        I2C1_WriteBufferDMA(0x45, data + i * 27, 0x9A, 27);
    }
    LCD_WriteReg(0x94, 0);
    LCD_WriteReg(0xE0, 0);
}

void TW8816_SetVideoMode(unsigned enable)
{
    unsigned reg = enable ? 0x40 : 0x44;
    if(LCD_ReadReg(0x2) != reg) {
        LCD_WriteReg(0x02, reg);
        LCD_WriteReg(0x2F, enable ? 0xE0 : 0xE6);
    }
}

void TW8816_ReinitPixelClock()
{
    LCD_WriteReg(0xB6, 0xB4);
    LCD_WriteReg(0xB2, 0x20);
    Delay(0x60000);
    LCD_WriteReg(0xB6, 0x34);
}

void TW8816_DisplayCharacter(unsigned pos, unsigned chr, unsigned attr)
{
    if (window == 1)
        pos += 430;
    LCD_WriteReg(0x94, (chr & 0x100) ? 0x80 : 0x00); // isRamFont
    LCD_WriteReg(0x95, pos >> 8);
    LCD_WriteReg(0x96, pos & 0xff);
    LCD_WriteReg(0x97, chr & 0xff);
    LCD_WriteReg(0x98, attr);
}

void TW8816_ClearDisplay()
{
    LCD_WriteReg(0x94, 2);
}

void TW8816_CreateMappedWindow(unsigned val, unsigned x, unsigned y, unsigned w, unsigned h)
{
    (void)w;
    (void)h;
    TW8816_SetWindow(val);
    LCD_WriteReg(0x9f, 0x01);
    x *= 12;
    y *= 18;
    LCD_WriteReg(0xa0, (0x30 & (y >> 4)) | (0x07 & (x >> 8)));
    LCD_WriteReg(0xa1, 0xff & x);
    LCD_WriteReg(0xa2, 0xff & y);
    TW8816_SetWindow(window);
}

void TW8816_SetWindow(unsigned i) {
    LCD_WriteReg(0x9e, i);
}
void TW8816_UnmapWindow(unsigned i)
{
    TW8816_SetWindow(i);
    LCD_WriteReg(0x9f, 0x00);
    TW8816_SetWindow(window);
}

void TW8816_Contrast(unsigned contrast)
{
    LCD_WriteReg(0x11, contrast);
}
void TW8816_Brightness(int brightness)
{
    LCD_WriteReg(0x10, brightness);
}
void TW8816_Sharpness(unsigned sharpness)
{
    int s = LCD_ReadReg(0x12);
    s = (s & ~0x0f) | sharpness;
    LCD_WriteReg(0x12, s);
}

void TW8816_Chroma(unsigned chromau, unsigned chromav)
{
    LCD_WriteReg(0x13, chromau);
    LCD_WriteReg(0x14, chromav);
}
