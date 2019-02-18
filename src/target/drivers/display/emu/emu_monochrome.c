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

#include <assert.h>

#include "common.h"
#include "gui/gui.h"
#include "target/drivers/mcu/emu/fltk.h"

//NOTE: LCD scaling used to be done here, but is now done in the fltk code
/*
 * // since devo10's screen is too small in emulator , we have it zoomed by 2 for both rows and columns
 * color = 0x0 means white, other value means black
 */
void LCD_DrawPixel(unsigned int color)
{
	if (gui.x < LCD_WIDTH && gui.y < LCD_HEIGHT) {	// both are unsigned, can not be < 0
		u8 c;
		// for emulator of devo 10, 0x0 means white while others mean black
		c = color ? 0x00 : 0xaa; // 0xaa is grey color(not dot)
		gui.image[3*(LCD_WIDTH * gui.y + gui.x)] = c;
		gui.image[3*(LCD_WIDTH * gui.y + gui.x) + 1] = c;
		gui.image[3*(LCD_WIDTH * gui.y + gui.x) + 2] = c;
        }
	// this must be executed to continue drawing in the next row
    gui.x++;
    if(gui.x > gui.xend) {
        gui.x = gui.xstart;
        gui.y += gui.dir;
    }
}

void LCD_Clear(unsigned int color) {
	(void)color;
	memset(gui.image, 0xaa, sizeof(gui.image));

}
