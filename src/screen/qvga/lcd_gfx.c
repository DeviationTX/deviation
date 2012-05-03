/***********************************
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


#include "target.h"
#include <stdlib.h>
#include <stdio.h>

/* helper functions */

#define swap(x, y) {int __tmp = x; x = y; y = __tmp;}

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

void LCD_DrawFastVLine(int16_t x, int16_t y, 
				 int16_t h, uint16_t color) {
    LCD_SetDrawArea(x, y, x, y + h);
    LCD_DrawStart();
    while(h--)
        LCD_DrawPixel(color);
    LCD_DrawStop();
}

void LCD_DrawFastHLine(u16 x, u16 y, u16 w, u16 color) {
    LCD_SetDrawArea(x, y, x + w, y);
    LCD_DrawStart();
    while(w--)
        LCD_DrawPixel(color);
    LCD_DrawStop();
}

// used to do circles and roundrects!
void fillCircleHelper(int16_t x0, int16_t y0, int16_t r,
			    uint8_t cornername, int16_t delta, uint16_t color) {

  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;

    if (cornername & 0x1) {
      LCD_DrawFastVLine(x0+x, y0-y, 2*y+1+delta, color);
      LCD_DrawFastVLine(x0+y, y0-x, 2*x+1+delta, color);
    }
    if (cornername & 0x2) {
      LCD_DrawFastVLine(x0-x, y0-y, 2*y+1+delta, color);
      LCD_DrawFastVLine(x0-y, y0-x, 2*x+1+delta, color);
    }
  }
}

void drawCircleHelper( int16_t x0, int16_t y0,
               int16_t r, uint8_t cornername, uint16_t color) {
  int16_t f     = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x     = 0;
  int16_t y     = r;

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f     += ddF_y;
    }
    x++;
    ddF_x += 2;
    f     += ddF_x;
    if (cornername & 0x4) {
      LCD_DrawPixelXY(x0 + x, y0 + y, color);
      LCD_DrawPixelXY(x0 + y, y0 + x, color);
    } 
    if (cornername & 0x2) {
      LCD_DrawPixelXY(x0 + x, y0 - y, color);
      LCD_DrawPixelXY(x0 + y, y0 - x, color);
    }
    if (cornername & 0x8) {
      LCD_DrawPixelXY(x0 - y, y0 + x, color);
      LCD_DrawPixelXY(x0 - x, y0 + y, color);
    }
    if (cornername & 0x1) {
      LCD_DrawPixelXY(x0 - y, y0 - x, color);
      LCD_DrawPixelXY(x0 - x, y0 - y, color);
    }
  }
}

/* end of helper functions */

// draw a circle outline
void LCD_DrawCircle(u16 x0, u16 y0, u16 r, u16 color)
{
  int16_t f = 1 - r;
  int16_t ddF_x = 1;
  int16_t ddF_y = -2 * r;
  int16_t x = 0;
  int16_t y = r;

  LCD_DrawPixelXY(x0, y0+r, color);
  LCD_DrawPixelXY(x0, y0-r, color);
  LCD_DrawPixelXY(x0+r, y0, color);
  LCD_DrawPixelXY(x0-r, y0, color);

  while (x<y) {
    if (f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;
  
    LCD_DrawPixelXY(x0 + x, y0 + y, color);
    LCD_DrawPixelXY(x0 - x, y0 + y, color);
    LCD_DrawPixelXY(x0 + x, y0 - y, color);
    LCD_DrawPixelXY(x0 - x, y0 - y, color);
    LCD_DrawPixelXY(x0 + y, y0 + x, color);
    LCD_DrawPixelXY(x0 - y, y0 + x, color);
    LCD_DrawPixelXY(x0 + y, y0 - x, color);
    LCD_DrawPixelXY(x0 - y, y0 - x, color);
    
  }
}

void LCD_FillCircle(u16 x0, u16 y0, u16 r, u16 color)
{
    LCD_DrawFastVLine(x0, y0-r, 2*r+1, color);
    fillCircleHelper(x0, y0, r, 3, 0, color);
}


// bresenham's algorithm - thx wikpedia
void LCD_DrawLine(u16 x0, u16 y0, u16 x1, u16 y1, u16 color)
{
  int16_t steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    swap(x0, y0);
    swap(x1, y1);
  }

  if (x0 > x1) {
    swap(x0, x1);
    swap(y0, y1);
  }

  int16_t dx, dy;
  dx = x1 - x0;
  dy = abs(y1 - y0);

  int16_t err = dx / 2;
  int16_t ystep;

  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }

  for (; x0<=x1; x0++) {
    if (steep) {
      LCD_DrawPixelXY(y0, x0, color);
    } else {
      LCD_DrawPixelXY(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

// draw a rectangle
void LCD_DrawRect(u16 x, u16 y, u16 w, u16 h, u16 color)
{
  LCD_DrawFastHLine(x, y, w, color);
  LCD_DrawFastHLine(x, y+h-1, w, color);
  LCD_DrawFastVLine(x, y, h, color);
  LCD_DrawFastVLine(x+w-1, y, h, color);
}

void LCD_FillRect(u16 x, u16 y, u16 w, u16 h, u16 color)
{
    LCD_SetDrawArea(x, y, x + w, y + h);
    LCD_DrawStart();
    w *= h;
    while(w--)
        LCD_DrawPixel(color);
    LCD_DrawStop();
}

// draw a rounded rectangle!
void LCD_DrawRoundRect(u16 x, u16 y, u16 w, u16 h, u16 r, u16 color)
{
  // smarter version
  LCD_DrawFastHLine(x+r  , y    , w-2*r, color); // Top
  LCD_DrawFastHLine(x+r  , y+h-1, w-2*r, color); // Bottom
  LCD_DrawFastVLine(  x    , y+r  , h-2*r, color); // Left
  LCD_DrawFastVLine(  x+w-1, y+r  , h-2*r, color); // Right
  // draw four corners
  drawCircleHelper(x+r    , y+r    , r, 1, color);
  drawCircleHelper(x+w-r-1, y+r    , r, 2, color);
  drawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
  drawCircleHelper(x+r    , y+h-r-1, r, 8, color);
}

// fill a rounded rectangle!
void LCD_FillRoundRect(u16 x, u16 y, u16 w, u16 h, u16 r, u16 color)
{
  // smarter version
  LCD_FillRect(x+r, y, w-2*r, h, color);

  // draw four corners
  fillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
  fillCircleHelper(x+r    , y+r, r, 2, h-2*r-1, color);
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
  int16_t a, b, y, last;

  // Sort coordinates by Y order (y2 >= y1 >= y0)
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }
  if (y1 > y2) {
    swap(y2, y1); swap(x2, x1);
  }
  if (y0 > y1) {
    swap(y0, y1); swap(x0, x1);
  }

  if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
    a = b = x0;
    if(x1 < a)      a = x1;
    else if(x1 > b) b = x1;
    if(x2 < a)      a = x2;
    else if(x2 > b) b = x2;
    LCD_DrawFastHLine(a, y0, b-a+1, color);
    return;
  }

  int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1,
    sa   = 0,
    sb   = 0;

  // For upper part of triangle, find scanline crossings for segments
  // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
  // is included here (and second loop will be skipped, avoiding a /0
  // error there), otherwise scanline y1 is skipped here and handled
  // in the second loop...which also avoids a /0 error here if y0=y1
  // (flat-topped triangle).
  if(y1 == y2) last = y1;   // Include y1 scanline
  else         last = y1-1; // Skip it

  for(y=y0; y<=last; y++) {
    a   = x0 + sa / dy01;
    b   = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    /* longhand:
    a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    LCD_DrawFastHLine(a, y, b-a+1, color);
  }

  // For lower part of triangle, find scanline crossings for segments
  // 0-2 and 1-2.  This loop is skipped if y1=y2.
  sa = dx12 * (y - y1);
  sb = dx02 * (y - y0);
  for(; y<=y2; y++) {
    a   = x1 + sa / dy12;
    b   = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    /* longhand:
    a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
    b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
    */
    if(a > b) swap(a,b);
    LCD_DrawFastHLine(a, y, b-a+1, color);
  }
}

#ifdef HAS_FS
void LCD_DrawWindowedImageFromFile(u16 x, u16 y, const char *file, u16 w, u16 h, u16 x_off, u16 y_off)
{
    u16 i, j;
    FILE *fh;
    u8 transparent = 0;
    u8 buf[0x46];
    fh = fopen(file, "r");
    u32 img_w, img_h, offset, compression;

    if(fread(buf, 0x46, 1, fh) != 1 || buf[0] != 'B' || buf[1] != 'M')
    {
    	printf("DEBUG: LCD_DrawWindowedImageFromFile: Buffer read issue?\n");
        return;
    }
    compression = *((u32 *)(buf + 0x1e));
    if(*((u16 *)(buf + 0x1a)) != 1      /* 1 plane */
       || *((u16 *)(buf + 0x1c)) != 16  /* 16bpp */
       || (compression != 0 && compression != 3)  /* BI_RGB or BI_BITFIELDS */
      )
    {
    	printf("DEBUG: LCD_DrawWindowedImageFromFile: BMP Format not correct\n");
    	return;
    }
    if(compression == 3)
    {
        if(*((u16 *)(buf + 0x36)) == 0x7c00 
           && *((u16 *)(buf + 0x3a)) == 0x03e0
           && *((u16 *)(buf + 0x3e)) == 0x001f
           && *((u16 *)(buf + 0x42)) == 0x8000)
        {
            transparent = 1;
        } else if(*((u16 *)(buf + 0x36)) != 0xf800 
           || *((u16 *)(buf + 0x3a)) != 0x07e0
           || *((u16 *)(buf + 0x3e)) != 0x001f)
        {
            printf("DEBUG: LCD_DrawWindowedImageFromFile: BMP Format not correct second check\n");
            return;
        }
    }
    offset = *((u32 *)(buf + 0x0a));
    img_w = *((u32 *)(buf + 0x12));
    img_h = *((u32 *)(buf + 0x16));
    if(w + x_off > img_w || h + y_off > img_h)
    {
    	printf("DEBUG: LCD_DrawWindowedImageFromFile: Dimensions asked for are out of bounds\n");
        return;
    }
    if(w == 0)
        w = img_w;
    if(h == 0)
        h = img_h;

    offset += (img_w * y_off + x_off) * 2;
    fseek(fh, offset, SEEK_SET);
    LCD_DrawStart();
    /* Bitmap start is at lower-left corner */
    for (j = 0; j < h; j++) {
        if(transparent) {
            for (i = 0; i < w; i++ ) {
                u16 color;
                fread(&color, 2, 1, fh);
                if((color & 0x8000)) {
                    //convert 1555 -> 565
                    color = ((color & 0x7fe0) << 1) | (color & 0x1f);
                    LCD_DrawPixelXY(x + i, y + h - j - 1, color);
                }
            }
        } else {
            LCD_SetDrawArea(x, y + h - j - 1, x + w - 1, y + h - j);
            for (i = 0; i < w; i++ ) {
                u16 color;
                fread(&color, 2, 1, fh);
                LCD_DrawPixel(color);
            }
        }
        if(w < img_w) {
            fseek(fh, 2 * (img_w - w), SEEK_CUR);
        }
    }
    LCD_DrawStop();
    fclose(fh);
}

void LCD_DrawImageFromFile(u16 x, u16 y, const char *file)
{
    LCD_DrawWindowedImageFromFile(x, y, file, 0, 0, 0, 0);
}
#endif
