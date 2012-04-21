/*
    This project is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Foobar is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "target.h"
#include "fonts_qvga.c"

void lcd_draw_pixel(unsigned int color);
void lcd_drawstart(void);
void lcd_drawstop(void);
void lcd_set_draw_area(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1);

static struct {
    const struct FONT_DEF *font;
    unsigned int x_start;
    unsigned int x;
    unsigned int y;
} cur_str = { &Fonts[4], 0, 0, 0};

void LCD_PrintCharXY(unsigned int x, unsigned int y, char c)
{
  u8 column[cur_str.font->width];
  u8 row, col;
  // Check if the requested character is available
  if ((c >= cur_str.font->first_char) && (c <= cur_str.font->last_char))
  {
    // Retrieve appropriate columns from font data
    for (col = 0; col < cur_str.font->width; col++)
    {
      // Get's first column of appropriate character
      column[col] = cur_str.font->font_table[((c - 32) * cur_str.font->width) + col];
    }
  }
  else
  {    
    // Requested characer is not available in this font ... send a space instead
    for (col = 0; col < cur_str.font->width; col++)
    {
      column[col] = 0xFF;    // Send solid space
    }
  }

  lcd_set_draw_area(x, y, x+cur_str.font->width-1, y+cur_str.font->height);
  lcd_drawstart();
  // Render each column
  for (row = 8-cur_str.font->height; row < 8; row++)
  {
    for (col = 0; col < cur_str.font->width; col++)
    {
      u8 color;
      color = (column[col] << (8 - (row + 1)));     // Shift current row bit left
      if(color & 0x80) {
          lcd_draw_pixel(0xffff);
      } else {
          lcd_draw_pixel(0);
      }
    }
  }
  lcd_drawstop();
}

void LCD_SetFont(unsigned int idx)
{
    int count = sizeof(Fonts) / sizeof(struct FONT_DEF);
    if(idx >= count)
        return;
    cur_str.font = &Fonts[idx];
}

void LCD_SetXY(unsigned int x, unsigned int y)
{
    cur_str.x_start = x;
    cur_str.x = x;
    cur_str.y = y;
}

void LCD_PrintStringXY(unsigned int x, unsigned int y, const char *str)
{
    LCD_SetXY(x, y);
    while(*str != 0) {
        LCD_PrintChar(*str);
        str++;
    }
}

void LCD_PrintString(const char *str)
{
    while(*str != 0) {
        LCD_PrintChar(*str);
        str++;
    }
}

void LCD_PrintChar(const char c)
{
    if(c == '\n') {
        cur_str.x = cur_str.x_start;
        cur_str.y += cur_str.font->height + 2;
    } else {
        LCD_PrintCharXY(cur_str.x, cur_str.y, c);
        cur_str.x += cur_str.font->width + 1;
    }
}

void LCD_Clear(unsigned int color){
        uint16_t zeile, spalte;
        lcd_set_draw_area(0, 0, (320-1), (240-1));
        lcd_drawstart();
        for(zeile = 0; zeile < 240; zeile++){
                for(spalte = 0; spalte < 320; spalte++){
                        lcd_draw_pixel(color);
                }
        }
        lcd_drawstop();

        return;
}

