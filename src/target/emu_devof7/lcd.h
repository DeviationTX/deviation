/**
 * Since we have some common LCD values we need to define some parts of the LCD here
 **/

#ifndef LCD_H
#define LCD_H

#define CHAR_WIDTH 12
#define CHAR_HEIGHT 18
#define RANGE_TABLE_SIZE 20

struct FAT FontFAT;

struct font_def 
{
        u8 idx;
        u8 height;          /* Character height for storage        */
        const u8 *data;
        u16 range[2 * (RANGE_TABLE_SIZE + 1)];  /* Array containing the ranges of supported characters */
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
