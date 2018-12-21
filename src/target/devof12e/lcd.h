/**
 * Since we have some common LCD values we need to define some parts of the LCD here
 **/

#ifndef LCD_H
#define LCD_H


void TW8816_Init();
void TW8816_LoadFont(u8 *data, unsigned offset, unsigned count);
void TW8816_SetVideoMode(unsigned enable);
void TW8816_DisplayCharacter(u16 pos, u8 chr, u16 attr);
void TW8816_ClearDisplay(u8 color);
void TW8816_SetWindow(unsigned i);
void TW8816_CreateMappedWindow(unsigned val, unsigned x, unsigned y, unsigned w, unsigned h);
void TW8816_UnmapWindow(unsigned i);
void TW8816_LoadColor(u8 index, u8 color);
u32 TW8816_map_char(u32);
void TW8816_Contrast(unsigned contrast);
void TW8816_Brightness(int brightness);
void TW8816_Sharpness(unsigned sharpness);
void TW8816_Chroma(unsigned chromau, unsigned chromav);
void TW8816_SetVideoChannel(int ch);
void TW8816_EnableVideo(int on);
u8 TW8816_GetVideoStandard();
void TW8816_SetVideoStandard(u8 standard);

#define LCD_ALIGN_LEFT      0
#define LCD_ALIGN_CENTER    1
#define LCD_ALIGN_RIGHT     2

#define LCD_FORCOLOR_MASK 0x7
#define LCD_BACKCOLOR_MASK (0x7 << 4)
#define LCD_BACKCOLOR_OFFSET 4
#define LCD_BLINK 0x8
#define LCD_FONT_RAM 0x8000
#define LCD_ITALIC 0x200
#define LCD_UNDERLINE 0x400
#define LCD_BOARDER 0x800

extern struct FAT FontFAT;

struct font_def 
{
	u8 idx;
    u8 zoom;          /* Character size relative to a 1x1 character */
};
struct font_str {
    struct font_def font;
    unsigned int x_start;
    unsigned int x;
    unsigned int y;
    u8           style;
    u8           color;
};
extern struct font_str cur_str;
#endif //LCD_H
