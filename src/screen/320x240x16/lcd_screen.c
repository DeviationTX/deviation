/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "common.h"
#include <stdlib.h>
#include <stdio.h>

void LCD_Clear(unsigned int color){
        uint16_t zeile, spalte;
        LCD_DrawStart(0, 0, (320-1), (240-1), DRAW_NWSE);
        for(zeile = 0; zeile < 240; zeile++){
                for(spalte = 0; spalte < 320; spalte++){
                        LCD_DrawPixel(color);
                }
        }
        LCD_DrawStop();

        return;
}

u8 LCD_ImageIsTransparent(const char *file)
{
    FILE *fh;
    u8 buf[0x46];
    fh = fopen(file, "r");
    if(! fh) {
        return 0;
    }
    u32 compression;

    if(fread(buf, 0x46, 1, fh) != 1 || buf[0] != 'B' || buf[1] != 'M')
    {
    	printf("DEBUG: LCD_ImageIsTransparent: Buffer read issue?\n");
        fclose(fh);
        return 0;
    }
    fclose(fh);
    compression = *((u32 *)(buf + 0x1e));
    if(compression == 3)
    {
        if(*((u16 *)(buf + 0x36)) == 0x7c00 
           && *((u16 *)(buf + 0x3a)) == 0x03e0
           && *((u16 *)(buf + 0x3e)) == 0x001f
           && *((u16 *)(buf + 0x42)) == 0x8000)
        {
            return 1;
        }
    }
    return 0;
}

u8 LCD_ImageDimensions(const char *file, u16 *w, u16 *h)
{
    FILE *fh;
    u8 buf[0x1a];
    fh = fopen(file, "r");
    if(! fh) {
        return 0;
    }

    if(fread(buf, 0x1a, 1, fh) != 1 || buf[0] != 'B' || buf[1] != 'M')
    {
        fclose(fh);
    	printf("DEBUG: LCD_ImageDimensions: Buffer read issue?\n");
        return 0;
    }
    fclose(fh);
    *w = *((u32 *)(buf + 0x12));
    *h = *((u32 *)(buf + 0x16));
    return 1;
}

void LCD_DrawWindowedImageFromFile(u16 x, u16 y, const char *file, s16 w, s16 h, u16 x_off, u16 y_off)
{
    u16 i, j;
    FILE *fh;
    u8 transparent = 0;
    u8 row_has_transparency = 0;
    u8 buf[320 * 2];

    if (w == 0 || h == 0)
        return;

    fh = fopen(file, "rb");
    if(! fh) {
    	printf("DEBUG: LCD_DrawWindowedImageFromFile: Image not found: %s\n", file);
        if (w > 0 && h > 0)
            LCD_FillRect(x, y, w, h, 0);
        return;
    }
    setbuf(fh, 0);
    u32 img_w, img_h, offset, compression;

    if(fread(buf, 0x46, 1, fh) != 1 || buf[0] != 'B' || buf[1] != 'M')
    {
        fclose(fh);
    	printf("DEBUG: LCD_DrawWindowedImageFromFile: Buffer read issue?\n");
        return;
    }
    compression = *((u32 *)(buf + 0x1e));
    if(*((u16 *)(buf + 0x1a)) != 1      /* 1 plane */
       || *((u16 *)(buf + 0x1c)) != 16  /* 16bpp */
       || (compression != 0 && compression != 3)  /* BI_RGB or BI_BITFIELDS */
      )
    {
        fclose(fh);
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
            fclose(fh);
            printf("DEBUG: LCD_DrawWindowedImageFromFile: BMP Format not correct second check\n");
            return;
        }
    }
    offset = *((u32 *)(buf + 0x0a));
    img_w = *((u32 *)(buf + 0x12));
    img_h = *((u32 *)(buf + 0x16));
    if(w < 0)
        w = img_w;
    if(h < 0)
        h = img_h;
    if((u16)w + x_off > img_w || (u16)h + y_off > img_h)
    {
    	printf("DEBUG: LCD_DrawWindowedImageFromFile: Dimensions asked for are out of bounds\n");
        printf("size: (%d x %d) bounds(%d x %d)\n", (u16)img_w, (u16)img_h, (u16)(w + x_off), (u16)(h + y_off));
        fclose(fh);
        return;
    }

    offset += (img_w * (img_h - (y_off + h)) + x_off) * 2;
    fseek(fh, offset, SEEK_SET);
    LCD_DrawStart(x, y, x + w - 1, y + h - 1, DRAW_SWNE);
    /* Bitmap start is at lower-left corner */
    for (j = 0; j < h; j++) {
        if (fread(buf, 2 * w, 1, fh) != 1)
            break;
        u16 *color = (u16 *)buf;
        if(transparent) {
            u8 last_pixel_transparent = row_has_transparency;
            row_has_transparency = 0;
            for (i = 0; i < w; i++ ) {
                if((*color & 0x8000)) {
                    //convert 1555 -> 565
                    u16 c = ((*color & 0x7fe0) << 1) | (*color & 0x1f);
                    if(last_pixel_transparent) {
                        LCD_DrawPixelXY(x + i, y + h - j - 1, c);
                        last_pixel_transparent = 0;
                    } else {
                        LCD_DrawPixel(c);
                    }
                } else {
                    //When we see a transparent pixel, the next real pixel
                    // will need to be drawn with XY coordinates
                    row_has_transparency = 1;
                    last_pixel_transparent = 1;
                }
                color++;
            }
        } else {
            for (i = 0; i < w; i++ ) {
                LCD_DrawPixel(*color++);
            }
        }
        if((u16)w < img_w) {
            fseek(fh, 2 * (img_w - w), SEEK_CUR);
        }
    }
    LCD_DrawStop();
    fclose(fh);
}

void LCD_DrawImageFromFile(u16 x, u16 y, const char *file)
{
    LCD_DrawWindowedImageFromFile(x, y, file, -1, -1, 0, 0);
}
