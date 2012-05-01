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
struct guiButton GUI_Button_Array[256];

int GUI_CreateButton(u16 x, u16 y, u16 width, u16 height, const char *text, void (*CallBack)(int objID)) {
	struct guiBox buttonBox;
	struct guiImage buttonImage;
	struct guiButton button;
	struct guiObject objButton;

	buttonBox.x = x;
	buttonBox.y = y;
	buttonBox.width = width;
	buttonBox.height = height;
	buttonImage.file = "gui.bmp";
	buttonImage.x_off = 0;
	buttonImage.y_off = 0;
	buttonBox.image = buttonImage;
	objButton.Type = Button;
	objButton.CallBack = *CallBack;

	button.box = buttonBox;
	button.text = text;

	int objLoc = GUI_GetFreeObj();
	int objButtonLoc = GUI_GetFreeButtonObj();
	if (objLoc == -1)
		return -1;
	if (objButtonLoc == -1)
		return -1;
	objButton.GUIID = objLoc;
	objButton.TypeID = objButtonLoc;

	GUI_Array[objLoc] = objButton;
	GUI_Button_Array[objButtonLoc] = button;
	return objLoc;
}
void GUI_DrawObjects(void) {
	struct guiBox buttonBox;
	struct guiImage buttonImage;

	int i;
	for (i=0;i<256;i++) {
		if (GUI_Array[i].CallBack != 0) {
			buttonBox = GUI_Button_Array[GUI_Array[i].TypeID].box;
			buttonImage = GUI_Button_Array[GUI_Array[i].TypeID].box.image;
			LCD_DrawWindowedImageFromFile(buttonBox.x, buttonBox.y, buttonImage.file, buttonBox.width, buttonBox.height, buttonImage.x_off, buttonImage.y_off);
			LCD_PrintStringXY(buttonBox.x, (buttonBox.y + ((buttonBox.height/2)-4)), GUI_Button_Array[GUI_Array[i].TypeID].text);
		}
	}
}
void GUI_RemoveObj(int objID) {
	if (GUI_Array[objID].Type == Button) {
		struct guiObject blankObj;
		struct guiButton blankButton;
		blankObj.CallBack = 0;
		blankObj.GUIID = 0;
		blankObj.Type = 0;
		blankObj.TypeID = 0;
		blankButton.text = "";
		GUI_Button_Array[GUI_Array[objID].TypeID] = blankButton;
		GUI_Array[objID] = blankObj;
	}
}
int GUI_GetFreeObj(void) {
	int i;
	for (i=0;i<256;i++) {
		if (GUI_Array[i].CallBack == 0) {
			return i;
		}
	}
	return -1;
}

int GUI_GetFreeButtonObj(void) {
	int i;
	for (i=0;i<256;i++) {
		if (GUI_Button_Array[i].box.width == 0) {
			return i;
		}
	}
	return -1;
}

void GUI_DrawScreen(void) {
	/*
	 * First we need to draw the main background
	 *  */
	LCD_DrawWindowedImageFromFile(0, 0, "devo8.bmp", 0, 0, 0, 0);
	/*
	 * Then we need to draw any supporting GUI
	 */
	GUI_DrawObjects();
}

void GUI_CheckTouch(struct touch coords) {
	int i;
	int calibrateX = 0;	/* Placeholder for X calibration offset */
	int calibrateY = 0;	    /* Placeholder for Y calibration offset */
	for (i=0;i<256;i++) {
		struct guiObject currentObject = GUI_Array[i];
		if (currentObject.CallBack != 0) {
			struct guiButton button = GUI_Button_Array[currentObject.TypeID];
			if (coords.x >= (button.box.x + calibrateX) &&
				coords.x <= ((button.box.width + button.box.x) + calibrateX) &&
				coords.y >= (button.box.y + calibrateY) &&
				coords.y <= ((button.box.height + button.box.y) + calibrateY)) {
					currentObject.CallBack(i);
			}
		}
	}
}

