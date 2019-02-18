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
#include "screen/font.h"

#if SUPPORT_MULTI_LANGUAGE
#define CHAR_CACHE_SIZE 150
#define LOC_CHAR_STARTS 50
u16 loadedchars[CHAR_CACHE_SIZE];
static u8 height;
static u16 load_char_font(u32 c);

#define FONT_NAME_LEN 9
char FontName[FONT_NAME_LEN] = {0};
#endif

void LCD_Init()
{
    TW8816_Init();
}

void LCD_Clear(unsigned int color) {
    (void)color;
    //printf("Clearing display\n");
    TW8816_ClearDisplay();
#if SUPPORT_MULTI_LANGUAGE
    memset(loadedchars, 0, sizeof(loadedchars));
#endif
}

void LCD_PrintCharXY(unsigned int x, unsigned int y, u32 c)
{
    c = TW8816_map_char(c);
#if SUPPORT_MULTI_LANGUAGE
    if (c > 0x300 + LOC_CHAR_STARTS) {
        if (height == 0) {
            int ok = open_font(FontName);
            if (ok) {
                height = get_height();
            }
        }
        if (height)
            c = load_char_font(c);
        else
            c = '?';
    }
#endif
    if (x >= LCD_WIDTH)
        x = LCD_WIDTH - 1;
    u16 pos = ((y >> 1)*(LCD_WIDTH>>1)) + (x>>1);
    //printf("%02x(%c): %d, %d, %d\n", c, c, x, y, pos);
    TW8816_DisplayCharacter(pos, c, 7);
}

static const struct font_def default_font = {2, 2};
struct font_str cur_str;

u8 LCD_SetFont(unsigned int idx)
{
    (void)idx;
    cur_str.font = default_font;
    return 1;
}

u8 FONT_GetFromString(const char *value)
{
    (void)value;
#if SUPPORT_MULTI_LANGUAGE
    // We take the first font as the font name from display.ini
    if (value[0] != '\0' && FontName[0] == '\0') {
        strlcpy(FontName, value, sizeof(FontName));
    }
#endif
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
            TW8816_LoadFont(font_map, 200, 6 * 4);
            for (int i = 0; i < 24; i++) {
                TW8816_DisplayCharacter(i, 0x300 + 200 + i, 7);
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

#if SUPPORT_MULTI_LANGUAGE
u16 load_char_font(u32 c)
{
    u8 width;
    u8 font[CHAR_BUF_SIZE];
    u8 lcdfont[27];
    int idx;

    char_read(font, c, &width);
    if (width == 0) {
        return (u16)'?';
    }

    for (idx = 0; idx < CHAR_CACHE_SIZE; idx++)
    {
        if (loadedchars[idx] == 0)  // empty slot
            break;
        if (loadedchars[idx] == (u16)c) {
            return 0x300 + 50 + idx;
        }
    }

    if (idx == CHAR_CACHE_SIZE) {
        printf("Char cache is over");
        return '~';
    }

    memset(lcdfont, 0, sizeof(lcdfont));

    u8 *offset = &font[0];

    int start, end;

    if (height >= CHAR_HEIGHT) {
        start = 0;
        end = CHAR_HEIGHT;
    } else {
        start = CHAR_HEIGHT - height;
        end = start + height;
    }

    if (width > CHAR_WIDTH) {
        width = CHAR_WIDTH;
    }

    // convert font to tw8816 format
    for (int x = 0; x < width; x++)
    {
        const u8 *data = offset++;
        u8 bit = 0;
        // Data is right aligned, drawn top to bottom
        for (int y = start; y < end; y++)
        {
            if (bit == 8) {
                data = offset++;
                bit = 0;
            }
            if (*data & (1 << bit)) {
                // there are 3 bytes across, contianing a total of 2 rows
                int byte = y / 2 * 3 + x / 4;
                // each byte is  4pixels wide by 2 rows tall
                int bit = 4 * (y & 1) + 3 - (x & 0x3);
                lcdfont[byte] |= 1 << bit;
            }
            bit++;
        }
    }

    // load into memory
    TW8816_LoadFont(lcdfont, 50 + idx, 1);
    loadedchars[idx] = (u16)c;

    return 0x300 + 50 + idx;
}
#endif
