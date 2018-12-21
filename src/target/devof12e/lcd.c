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

#include "common.h"
#include "lcd.h"

static u8  backcolor;
static const struct font_def default_font = { 2, 2 };
struct font_str cur_str;

// cache the color of the device
static u8 colors[16];
static u8 last_color;

u8 LCD_GetColor(unsigned int color)
{
    u8 i;
    // first convert the color from RGB565 to RGB332
    u8 dcolor = ((color & 0x18) >> 3) |
                ((color & 0x700) >> 6) |
                ((color & 0xe000) >> 8);

    // if there is a slot with existing color
    for (i = 0; i < last_color; i++) {
        if (colors[i] == dcolor)
            return i;
    }

    // if not found, but empty slot available
    if (last_color < 16) {
        colors[last_color] = dcolor;
        TW8816_LoadColor(last_color, dcolor);
        last_color++;
        return last_color - 1;
    }

    return 1; // return second color
}

void LCD_Init()
{
    TW8816_Init();
}

void LCD_Clear(unsigned int color)
{
    last_color = 0;
    backcolor = LCD_GetColor(color);
    TW8816_ClearDisplay(backcolor);
}

void LCD_PrintCharXY(unsigned int x, unsigned int y, u32 c)
{
    u16 attr;

    c = TW8816_map_char(c);

    if (x >= LCD_WIDTH)
        x = LCD_WIDTH - 1;
    u16 pos = ((y >> 1)*(LCD_WIDTH>>1)) + (x>>1);

    attr = 0;
    if (cur_str.style & FONT_ITALIC)
        attr |= LCD_ITALIC;
    if (cur_str.style & FONT_UNDERLINE)
        attr |= LCD_UNDERLINE;
    if (cur_str.style & FONT_BLINK)
        attr |= LCD_BLINK;

    if (cur_str.style & FONT_INVERTED) {
        attr |= cur_str.color << LCD_BACKCOLOR_OFFSET;
        attr |= backcolor;
    }
    else {
        attr |= backcolor << LCD_BACKCOLOR_OFFSET;
        attr |= cur_str.color;
    }

    if ((c & 0xFF00) >= 0x300) {
        attr |= LCD_FONT_RAM;
    }

    TW8816_DisplayCharacter(pos, (u8)(c & 0xff), attr);
}

void LCD_SetFontStyle(u8 style)
{
    cur_str.style = style;
}

void LCD_SetFontColor(u16 color)
{
    cur_str.color = LCD_GetColor(color);
}

u8 LCD_SetFont(unsigned int idx)
{
    (void)idx;
    cur_str.font = default_font;
    return 1;
}

u8 FONT_GetFromString(const char *value)
{
    (void)value;
    return 1;
}

#ifndef EMULATOR
void LCD_DrawStart(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, enum DrawDir _dir)
{
    (void) x0; (void) y0; (void) x1; (void) y1; (void) _dir;
}

void LCD_DrawStop(void)
{

}
#endif

void LCD_ShowVideo(u8 enable)
{
    TW8816_SetVideoMode(enable);
}

extern u8 font_map[27 * 6* 4];
extern u8 window;

void LCD_CreateMappedWindow(unsigned val, unsigned x, unsigned y, unsigned w, unsigned h)
{
    val = val == 1 ? 0 : 1;
    TW8816_CreateMappedWindow(val, x, y, w, h);
}

void LCD_UnmapWindow(unsigned val)
{
    val = val == 1 ? 0 : 1;
    TW8816_UnmapWindow(val);
}

void LCD_SetMappedWindow(unsigned val)
{
    if (val != 0) {
        TW8816_SetWindow(0);
        memset(font_map, 0, sizeof(font_map));
    } else {
        if (window < 4) {
            TW8816_SetWindow(1);
            TW8816_LoadFont(font_map, 200, 6 * 4);
            u8 color = LCD_GetColor(0xffff);
            for (int i = 0; i < 24; i++) {
                TW8816_DisplayCharacter(i,
                    0x300 + 200 + i,
                    LCD_FONT_RAM | color);
            }
        } else {
            TW8816_LoadFont(font_map, window - 4, 1);
        }
        TW8816_SetWindow(1);
    }
    window = val;
}

#ifndef EMULATOR
void LCD_Contrast(unsigned contrast)
{
    (void)contrast;
}

void VIDEO_Contrast(int contrast)
{
    int c = (int)contrast * 128 / 10 + 128;
    if (c < 0)
        c = 0;
    if (c > 255)
        c = 255;
    TW8816_Contrast(c);
}

void VIDEO_Brightness(int brightness)
{
    int b = (int)brightness * 128 / 10;
    if (b < -128)
        b = -128;
    if (b > 127)
        b = 127;
    TW8816_Brightness(b);
}

void VIDEO_Chroma(unsigned chromau, unsigned chromav)
{
    chromau *= 20;
    chromav *= 20;
    TW8816_Chroma(chromau, chromav);
}

void VIDEO_SetChannel(int ch)
{
    TW8816_SetVideoChannel(ch);
}

void VIDEO_Enable(int on)
{
    TW8816_EnableVideo(on);
}

u8 VIDEO_GetStandard()
{
    return TW8816_GetVideoStandard();
}

void VIDEO_SetStandard(u8 standard)
{
    TW8816_SetVideoStandard(standard);
}
#endif
