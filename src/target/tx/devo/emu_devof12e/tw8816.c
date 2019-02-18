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

#include <assert.h>

#include "common.h"
#include "config/display.h"
#include "gui/gui.h"
#include "target/drivers/mcu/emu/fltk.h"
#include "lcd.h"
#include "tw8816_font.h"
#include "tw8816_2_font.h"
#include "tw8816_3_font.h"

#define HEIGHT(x) x.height

extern u8 window;
u8 window_mult;
unsigned window_x, window_y;

struct rgb {
    u8 r;
    u8 g;
    u8 b;
};
static const struct rgb background = {0x00, 0x00, 0xff};
static const struct rgb foreground = {0xff, 0xff, 0xff};

#define RANGE_TABLE_SIZE 20

struct font_def_rom
{
        u8 idx;
        u8 height;          /* Character height for storage        */
        u8 zoom;
        const u8 *data;
        u16 range[2 * (RANGE_TABLE_SIZE + 1)];  /* Array containing the ranges of supported characters */
};

static struct font_def_rom tiny_font;
static struct font_def_rom default_font;
static struct font_def_rom large_font;

/*
 * // since devo10's screen is too small in emulator , we have it zoomed by 2 for both rows and columns
 * color = 0x0 means white, other value means black
 */
void LCD_DrawPixel(unsigned int color)
{
    if (gui.x < IMAGE_X && gui.y < IMAGE_Y)
    {   // both are unsigned, can not be < 0
        struct rgb c;
        // for emulator of devo 10, 0x0 means white while others mean black
        c = color ? foreground : background; // 0xaa is grey color(not dot)

        gui.image[3*(IMAGE_X * gui.y + gui.x) + 0]     = c.r;
        gui.image[3*(IMAGE_X * gui.y + gui.x) + 1]     = c.g;
        gui.image[3*(IMAGE_X * gui.y + gui.x) + 2]     = c.b;
    }
    // this must be executed to continue drawing in the next row
    gui.x++;
    if(gui.x > gui.xend) {
        gui.x = gui.xstart;
        gui.y += gui.dir;
    }
}

/*
 * Since we have a text based screen we need some of the text writing functions here
 */
static u8 get_char_range(u32 c, u32 *begin, u32 *end)
{
    u32 offset = 0;
    u32 pos = 5;
    u16 *range = default_font.range;
    while(1) {
        if (range[0] == 0 && range[1] == 0)
            break;
        if (c >= range[0] && c <= range[1]) {
            pos += 3 * (offset + c - range[0]);
        } else {
            offset += range[1] + 1 - range[0];
        }
        range += 2;
        pos += 4;
    }
    const u8 *font = default_font.data + pos;
    *begin = font[0] | (font[1] << 8) | (font[2] << 16);
    *end   = font[3] | (font[4] << 8) | (font[5] << 16);
    return 1;
}

static const u8 *char_offset(u32 c, u8 *width)
{
    u32 begin;
    u32 end;

    u8 row_bytes = ((default_font.height - 1) / 8) + 1;
    get_char_range(c, &begin, &end);
    *width = (end - begin) / row_bytes;
    return default_font.data + begin;
}

static void close_font()
{
}

static void open_font(struct font_def_rom *font, const u8 *data, int fontidx)
{
    font->height = *data;
    font->zoom = *data / CHAR_HEIGHT;
    font->idx = fontidx;
    font->data = data;
    int idx = 0;
    const u8 *f = data+1;
    while(1) {
        u16 start_c = f[0] | (f[1] << 8);
        u16 end_c = f[2] | (f[3] << 8);
        font->range[idx++] = start_c;
        font->range[idx++] = end_c;
        f+= 4;
        if (start_c == 0 && end_c == 0)
            break;
    }
}

static u8 font_ram[227 * 27]; // 227 characters, 27 bytes per character
static u32 characters[255]; // 20bit per character
static struct _window_info
{
    int Enabled;
    int start_characters;
    int H_start, V_start;
    u8 H_size, V_size;
    u8 H_spacing, V_spacing;
    u8 H_zoom, V_zoom;
}Windows[4]; // 4 window in total

extern void EMULCD_Init();
void TW8816_Init()
{
    memset(font_ram, 0, sizeof(font_ram));
    memset(characters, 0, sizeof(characters));
    memset(Windows, 0, sizeof(Windows));

    EMULCD_Init();

    open_font(&tiny_font,    tw8816_fon, 3);
    open_font(&default_font, tw8816_2_fon, 1);
    open_font(&large_font,   tw8816_3_fon, 2);

    Windows[0].H_start = 0;
    Windows[0].V_start = 0;

    Windows[0].H_zoom = 2;
    Windows[0].V_zoom = 2;

    Windows[0].H_size = LCD_WIDTH;
    Windows[0].V_size = LCD_HEIGHT;
    Windows[0].Enabled = 1;

    Windows[1].Enabled = 0;
}

void TW8816_LoadFont(u8 *data, unsigned offset, unsigned count)
{
   memcpy(&font_ram[offset * 27], data, 27 * count);
}

void TW8816_SetVideoMode(unsigned enable)
{
    printf("Video is %s\n", enable?"Enabled":"Disabled");
}

void TW8816_DisplayCharacter(u16 pos, unsigned chr, unsigned attr)
{
    (void)attr;

    int x, y;
    int col, row;
    u8 width;

    if (!Windows[window].Enabled)
        return;

    int index = window;
    pos = pos * 2;
    x = pos % Windows[index].H_size;
    y = (pos - x ) / Windows[index].H_size;
    y *= 2;
    x += Windows[index].H_start;
    y += Windows[index].V_start;

    int font_size = Windows[index].H_zoom;
    const u8 *offset;

    if (chr >= 0x300)
    {
        offset = &font_ram[(chr - 0x300) * 27];
        width = CHAR_WIDTH;

        for (int dx = 0; dx < CHAR_WIDTH; dx++) {
            for (int dy = 0; dy < CHAR_HEIGHT; dy++) {
                int byte = dy / 2 * 3 + dx / 4;
                int bit = 4 * (dy & 1) + 3 - (dx & 0x3);

                int c = (offset[byte] & (1 << bit)) ? 1 : 0;
                for (int my = 0; my < font_size; my++)
                    for (int mx = 0; mx < font_size; mx++)
                        LCD_DrawPixelXY((x * CHAR_WIDTH) + dx*font_size + mx,
                            (y * CHAR_HEIGHT) + dy*font_size+my, c);
            }
        }
    }
    else
    {
        // Check if the requested character is available
        offset = char_offset(chr, &width);

        if (! offset || ! width) {
            printf("Could not locate character U-%04x\n", (int)chr);
            return;
        }

        LCD_DrawStart(x * CHAR_WIDTH, y * CHAR_HEIGHT,
            (x+font_size) * CHAR_WIDTH, (y+font_size) * CHAR_HEIGHT,
            DRAW_NWSE);
        // First clean th area
        for(col = 0; col < font_size * CHAR_WIDTH; col++) {
            for(row = 0; row < font_size * CHAR_HEIGHT; row++) {
                LCD_DrawPixelXY((x * CHAR_WIDTH) + col, (y * CHAR_HEIGHT) + row, 0);
            }
        }

        for (col = 0; col < width; col++)
        {
            const u8 *data = offset++;
            u8 bit = 0;
            // Data is right aligned, drawn top to bottom
            for (row = 0; row < HEIGHT(default_font); ++row)
            {
                if (bit == 8) {
                    data = offset++;
                    bit = 0;
                }
                if (*data & (1 << bit)) {
                    LCD_DrawPixelXY((x * CHAR_WIDTH) + col, (y * CHAR_HEIGHT) + row, cur_str.color);
                }
                bit++;
            }
        }
        LCD_DrawStop();
    }
}

void TW8816_ClearDisplay()
{
    memset(characters, 0, sizeof(characters));

    for (unsigned i = 0; i < sizeof(gui.image); i+= 3) {
        gui.image[i] = background.r;
        gui.image[i+1] = background.g;
        gui.image[i+2] = background.b;
    }
}

void TW8816_SetWindow(unsigned i)
{
    (void)i;
}

void TW8816_CreateMappedWindow(unsigned val, unsigned x, unsigned y, unsigned w, unsigned h)
{
    val = 1 - val;
    Windows[val].H_start = x;
    Windows[val].V_start = y;

    Windows[val].H_size = w * 2;
    Windows[val].V_size = h * 2;
    Windows[val].H_zoom = 2;
    Windows[val].V_zoom = 2;

    Windows[val].Enabled = 1;
}

void TW8816_UnmapWindow(unsigned i)
{
    i = 1 - i;
    Windows[i].Enabled = 0;
}

void TW8816_Contrast(unsigned contrast)
{
    printf("%s: %d\n", __FUNCTION__, contrast);
}

void TW8816_Brightness(int brightness)
{
    printf("%s: %d\n", __FUNCTION__, brightness);
}

void TW8816_Sharpness(unsigned sharpness)
{
    printf("%s: %d\n", __FUNCTION__, sharpness);
}

void TW8816_Chroma(unsigned chromau, unsigned chromav)
{
    printf("%s: %d, %d\n", __FUNCTION__, chromau, chromav);
}

u8 TW8816_GetVideoStandard()
{
    printf("%s\n", __FUNCTION__);

    return 0; // not connected
}

void TW8816_SetVideoStandard(u8 standard)
{
    printf("%s: %d\n", __FUNCTION__, standard);
}

void TW8816_SetVideoChannel(int ch)
{
    printf("%s: %d\n", __FUNCTION__, ch);
}

void TW8816_EnableVideo(int on)
{
    printf("%s: %s\n", __FUNCTION__, on?"on":"off");
}
