/**
 * Since we have some common LCD values we need to define some parts of the LCD here
 **/

#ifndef LCD_H
#define LCD_H

#define LCD_ALIGN_LEFT      0
#define LCD_ALIGN_CENTER    1
#define LCD_ALIGN_RIGHT     2

struct FAT FontFAT;

struct {
    unsigned int x_start;
    unsigned int x;
    unsigned int y;
    u16          color;
} cur_str;

#endif //LCD_H