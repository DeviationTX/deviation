/**
 * Since we have some common LCD values we need to define some parts of the LCD here
 **/

#ifndef LCD_H
#define LCD_H


void TW8816_Init();
void TW8816_Reset();
void TW8816_ResetLoop();
void TW8816_LoadFont(u8 *data, unsigned offset, unsigned count);
void TW8816_SetVideoMode(unsigned enable);
void TW8816_ReinitPixelClock();
void TW8816_DisplayCharacter(unsigned pos, unsigned chr, unsigned attr);
void TW8816_ClearDisplay();
void TW8816_SetWindow(unsigned i);
void TW8816_CreateMappedWindow(unsigned val, unsigned x, unsigned y, unsigned w, unsigned h);
void TW8816_UnmapWindow(unsigned i);
u32 TW8816_map_char(u32);
void TW8816_Contrast(unsigned contrast);
void TW8816_Brightness(int brightness);
void TW8816_Sharpness(unsigned sharpness);
void TW8816_Chroma(unsigned chromau, unsigned chromav);
u8 TW8816_GetVideoStandard();
void TW8816_SetVideoStandard(u8 standard);

void LCD_WriteReg(unsigned reg, u8 val);
void LCD_WriteBuffer(u16 periphAddr, u8 *buffer, unsigned len);
u32 LCD_ReadReg(unsigned reg);

#define LCD_ALIGN_LEFT      0
#define LCD_ALIGN_CENTER    1
#define LCD_ALIGN_RIGHT     2

struct FAT FontFAT;

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
    u16          color;
};
extern struct font_str cur_str;
#endif //LCD_H
