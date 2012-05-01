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
#include "fonts.h"

static struct {
    const struct FONT_DEF *font;
    unsigned int x_start;
    unsigned int x;
    unsigned int y;
    u16          color;
} cur_str = { &Fonts[4], 0, 0, 0, 0xffff};

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

#ifdef TRANSPARENT_FONT
  LCD_DrawStart();
  for (row = 8-cur_str.font->height; row < 8; row++)
  {
    for (col = 0; col < cur_str.font->width; col++)
    {
      u8 color;
      color = (column[col] << (8 - (row + 1)));     // Shift current row bit left
      if(color & 0x80) {
          LCD_DrawPixelXY(x + col, y + row, cur_str.color);
      }
    }
  }
  LCD_DrawStop();
#else
  LCD_SetDrawArea(x, y, x+cur_str.font->width-1, y+cur_str.font->height);
  LCD_DrawStart();
  // Render each column
  for (row = 8-cur_str.font->height; row < 8; row++)
  {
    for (col = 0; col < cur_str.font->width; col++)
    {
      u8 color;
      color = (column[col] << (8 - (row + 1)));     // Shift current row bit left
      if(color & 0x80) {
          LCD_DrawPixel(cur_str.color);
      } else {
          LCD_DrawPixel(0);
      }
    }
  }
  LCD_DrawStop();
#endif
}

void LCD_SetFont(unsigned int idx)
{
    int i;
    for(i = 0; i <= idx; i++) {
        if(Fonts[i].width == 0)
            return;
    }
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
        LCD_SetDrawArea(0, 0, (320-1), (240-1));
        LCD_DrawStart();
        for(zeile = 0; zeile < 240; zeile++){
                for(spalte = 0; spalte < 320; spalte++){
                        LCD_DrawPixel(color);
                }
        }
        LCD_DrawStop();

        return;
}

