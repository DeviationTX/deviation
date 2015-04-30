/************************************
This is a our graphics core library, for all our displays. 
We'll be adapting all the
existing libaries to use this core to make updating, support 
and upgrading easier!

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above must be included in any redistribution
****************************************/


#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include "gui/gui.h"

void LCD_DrawFastVLine(int16_t x, int16_t y, 
                 int16_t h, uint16_t color) {
  (void) x;
  (void) y;
  (void) h;
  (void) color;
}

void LCD_DrawFastHLine(u16 x, u16 y, u16 w, u16 color) {
  (void) x;
  (void) y;
  (void) w;
  (void) color;
}

void LCD_DrawDashedHLine(int16_t x, int16_t y, 
             int16_t w, int16_t space, uint16_t color)
{
  (void) x;
  (void) y;
  (void) w;
  (void) space;
  (void) color;
}

void LCD_DrawDashedVLine(int16_t x, int16_t y, 
             int16_t h, int16_t space, uint16_t color) {
  (void) x;
  (void) y;
  (void) h;
  (void) space;
  (void) color;
}

// draw a circle outline
void LCD_DrawCircle(u16 x0, u16 y0, u16 r, u16 color)
{
  (void) x0;
  (void) y0;
  (void) r;
  (void) color;
}

void LCD_FillCircle(u16 x0, u16 y0, u16 r, u16 color)
{
  (void) x0;
  (void) y0;
  (void) r;
  (void) color;
}


// bresenham's algorithm - thx wikpedia
void LCD_DrawLine(u16 x0, u16 y0, u16 x1, u16 y1, u16 color)
{
  (void) x0;
  (void) y0;
  (void) x1;
  (void) y1;
  (void) color;
}

// draw a rectangle
void LCD_DrawRect(u16 x, u16 y, u16 w, u16 h, u16 color)
{
  (void) x;
  (void) y;
  (void) w;
  (void) h;
  (void) color;
}

void LCD_FillRect(u16 x, u16 y, u16 w, u16 h, u16 color)
{
  unsigned i, j;
  (void) color;

  for(i = 0; i < h; i++) {
    LCD_SetXY(x, y + i);
    for(j = 0; j < w; j++)
      LCD_PrintChar(' ');
  }
}

// draw a rounded rectangle!
void LCD_DrawRoundRect(u16 x, u16 y, u16 w, u16 h, u16 r, u16 color)
{
  (void) x;
  (void) y;
  (void) w;
  (void) h;
  (void) r;
  (void) color;
}

// fill a rounded rectangle!
void LCD_FillRoundRect(u16 x, u16 y, u16 w, u16 h, u16 r, u16 color)
{
  (void) x;
  (void) y;
  (void) w;
  (void) h;
  (void) r;
  (void) color;
}

// draw a triangle!
void LCD_DrawTriangle(u16 x0, u16 y0, u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
  LCD_DrawLine(x0, y0, x1, y1, color);
  LCD_DrawLine(x1, y1, x2, y2, color);
  LCD_DrawLine(x2, y2, x0, y0, color);
}

// fill a triangle!
void LCD_FillTriangle(u16 x0, u16 y0, u16 x1, u16 y1, u16 x2, u16 y2, u16 color)
{
  (void) x0;
  (void) y0;
  (void) x1;
  (void) y1;
  (void) x2;
  (void) y2;
  (void) color;
}

u8 LCD_ImageIsTransparent(const char *file)
{
  (void) file;  
  return 0;
}

u8 LCD_ImageDimensions(const char *file, u16 *w, u16 *h)
{
    (void) file;
    *w = 0;
    *h = 0;
    return 1;
}

void LCD_DrawWindowedImageFromFile(u16 x, u16 y, const char *file, s16 w, s16 h, u16 x_off, u16 y_off)
{
  (void) x;
  (void) y;
  (void) file;
  (void) w;
  (void) h;
  (void) x_off;
  (void) y_off;
}

void LCD_DrawImageFromFile(u16 x, u16 y, const char *file)
{
  (void) x;
  (void) y;
  (void) file;
}

void LCD_DrawRLE(const u8 *data, int len, u32 color)
{
  (void) data;
  (void) len;
  (void) color;
}

void LCD_DrawUSBLogo(int lcd_width, int lcd_height)
{
  u16 width, height;
  LCD_SetFont(2);
  LCD_SetFontColor(0xffff);
  LCD_GetStringDimensions(_tr("Deviation\n   USB"), &width, &height);
  printf("(%d, %d)- > (%d, %d)", lcd_width, lcd_height, width, height);
  LCD_PrintStringXY((lcd_width - width)/2, lcd_height/2-1, _tr("Deviation\n   USB"));
}
