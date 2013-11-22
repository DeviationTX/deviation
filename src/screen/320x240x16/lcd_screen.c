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
#include "_gui.h"

void LCD_Clear(unsigned int color){
        uint16_t zeile, spalte;
        LCD_DrawStart(0, 0, (LCD_WIDTH), (LCD_HEIGHT-1), DRAW_NWSE);
        for(zeile = 0; zeile < LCD_HEIGHT; zeile++){
                for(spalte = 0; spalte < LCD_WIDTH; spalte++){
                        LCD_DrawPixel(color);
                }
        }
        LCD_DrawStop();

        return;
}
