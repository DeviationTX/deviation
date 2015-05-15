/**
 * Since we have some common LCD values we need to define some parts of the LCD here
 **/

#ifndef LCD_H
#define LCD_H

u32 I2C1_WriteBufferDMA(u16 deviceId, u8 *dmaAddr, s16 periphMemAddr, int len);

void TW8816_Init();
void TW8816_Reset();
void TW8816_LoadFont();
void TW8816_SetVideoMode(unsigned enable);
void TW8816_ReinitPixelClock();
void TW8816_DisplayCharacter(unsigned pos, unsigned chr, unsigned attr);
void TW8816_ClearDisplay();
void TW8816_SetWindow(int i);
u32 TW8816_map_char(u32);


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
