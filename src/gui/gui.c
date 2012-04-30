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
#include "gui.h"

struct guiObject GUI_Array[256];

int GUI_CreateButton(u16 x, u16 y, u16 width, u16 height, const char *text, void (*CallBack)(void)) {
	struct guiBox buttonBox;
	struct guiImage buttonImage;
	struct guiObject objButton;

	buttonBox.x = x;
	buttonBox.y = y;
	buttonBox.width = width;
	buttonBox.height = height;
	buttonImage.file = "gui.bmp";
	buttonImage.x_off = 0;
	buttonImage.y_off = 0;

	objButton.Type = Button;
	objButton.CallBack = *CallBack;
	objButton.GUIID = 0;
	GUI_Array[0] = objButton;
	LCD_DrawWindowedImageFromFile(buttonBox.x, buttonBox.y, buttonImage.file, buttonBox.width, buttonBox.height, buttonImage.x_off, buttonImage.y_off);
	return 0;
}
void GUI_CheckTouch(struct touch coords) {

}

