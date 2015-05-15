/**
 * Since we have some common LCD values we need to define some parts of the LCD here
 **/

#ifndef LCD_H
#define LCD_H

#define LCD_ALIGN_LEFT      0
#define LCD_ALIGN_CENTER    1
#define LCD_ALIGN_RIGHT     2

struct FAT FontFAT;

struct font_def 
{
        u8 idx;
        u8 height;          /* Character height for storage        */
};
struct font_str {
    struct font_def font;
    unsigned int x_start;
    unsigned int x;
    unsigned int y;
    u16          color;
};
extern struct font_str cur_str;

u32 IA9211_map_char(u32 c);
#endif //LCD_H
