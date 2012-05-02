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
struct guiLabel GUI_Label_Array[256];
struct guiFrame GUI_Frame_Array[256];

void GUI_SetText(int GUIID,const char *text) {
	int TypeID = GUI_Array[GUIID].TypeID;
	switch (GUI_Array[GUIID].Type) {
		case Button:
		{
			sprintf(GUI_Button_Array[TypeID].text,"%s",text);
		}
		break;
		case Label:
		{
			sprintf(GUI_Label_Array[TypeID].text,"%s",text);
		}
		break;
		case Frame:
		break;
		case CheckBox:
		break;
		case Dropdown:
		break;
	}
}
char* GUI_GetText(int GUIID) {
	int TypeID = GUI_Array[GUIID].TypeID;
	switch (GUI_Array[GUIID].Type) {
		case Button:
		{
			return GUI_Button_Array[TypeID].text;
		}
		break;
		case Label:
		{
			return GUI_Label_Array[TypeID].text;
		}
		break;
		case Frame:
		break;
		case CheckBox:
		break;
		case Dropdown:
		break;
	}
}

int GUI_CreateLabel(u16 x, u16 y, const char *text) {
	struct guiBox labelBox;
	struct guiObject objLabel;
	struct guiLabel label;

	labelBox.x = x;
	labelBox.y = y;
	labelBox.width = 10;
	labelBox.height = 10;
	objLabel.Type = Label;
	objLabel.CallBack = 0x1; /* no call back yet for labels */
	label.box = labelBox;
	sprintf(label.text,"%s",text);

	int objLoc = GUI_GetFreeObj();
	int objLabelLoc = GUI_GetFreeGUIObj(Label);
	if (objLoc == -1)
		return -1;
	if (objLabelLoc == -1)
		return -1;
	objLabel.GUIID = objLoc;
	objLabel.TypeID = objLabelLoc;

	GUI_Array[objLoc] = objLabel;
	GUI_Label_Array[objLabelLoc] = label;
	return objLoc;
}
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
	sprintf(button.text,"%s",text);

	int objLoc = GUI_GetFreeObj();
	int objButtonLoc = GUI_GetFreeGUIObj(Button);
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

	int i;
	for (i=0;i<256;i++) {
		if (GUI_Array[i].CallBack != 0) {
			switch (GUI_Array[i].Type) {
				case Button:
				{
					struct guiBox buttonBox;
					struct guiImage buttonImage;
					buttonBox = GUI_Button_Array[GUI_Array[i].TypeID].box;
					buttonImage = GUI_Button_Array[GUI_Array[i].TypeID].box.image;
#ifdef HAS_FS
					LCD_DrawWindowedImageFromFile(buttonBox.x, buttonBox.y, buttonImage.file, buttonBox.width, buttonBox.height, buttonImage.x_off, buttonImage.y_off);
#endif
					LCD_PrintStringXY(buttonBox.x, (buttonBox.y + ((buttonBox.height/2)-4)), GUI_Button_Array[GUI_Array[i].TypeID].text);
				}
				break;
				case Label:
				{
					struct guiBox labelBox;
					labelBox = GUI_Label_Array[GUI_Array[i].TypeID].box;
					LCD_PrintStringXY(labelBox.x, labelBox.y, GUI_Label_Array[GUI_Array[i].TypeID].text);
				}
				break;
				case Frame:
				break;
				case CheckBox:
				break;
				case Dropdown:
				break;
			}
		}
	}
}
void GUI_RemoveObj(int objID) {
	struct guiObject blankObj;
	blankObj.CallBack = 0;
	blankObj.GUIID = 0;
	blankObj.Type = 0;
	blankObj.TypeID = 0;

	switch (GUI_Array[objID].Type) {
		case Button:
		{
			struct guiButton blankButton;
			sprintf(blankButton.text," ");
			GUI_Button_Array[GUI_Array[objID].TypeID] = blankButton;
		}
		break;
		case Label:
		{
			struct guiLabel blankLabel;
			sprintf(blankLabel.text," ");
			GUI_Label_Array[GUI_Array[objID].TypeID] = blankLabel;
		}
		break;
		case Frame:
		{
			struct guiFrame blankFrame;
			blankFrame.text = "";
			GUI_Frame_Array[GUI_Array[objID].TypeID] = blankFrame;
		}
		break;
		case CheckBox:
		break;
		case Dropdown:
		break;
	}
	GUI_Array[objID] = blankObj;
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

int GUI_GetFreeGUIObj(enum GUIType guiType) {
	int i;
	for (i=0;i<256;i++) {
		switch (guiType) {
			case Button:
				{
					if (GUI_Button_Array[i].box.width == 0) {
						return i;
					}
				}
				break;
			case Label:
				{
					if (GUI_Label_Array[i].box.width == 0) {
						return i;
					}
				}
				break;
			case Frame:
				{
					if (GUI_Frame_Array[i].box.width == 0) {
						return i;
					}
				}
				break;
			case CheckBox:
				{
				}
				break;
			case Dropdown:
				{
				}
				break;
		}
	}
	return -1;
}

void GUI_DrawScreen(void) {
	/*
	 * First we need to draw the main background
	 *  */
#ifdef HAS_FS
	LCD_DrawWindowedImageFromFile(0, 0, "devo8.bmp", 0, 0, 0, 0);
#endif
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
			switch (currentObject.Type) {
				case Button:
				{
					struct guiButton button = GUI_Button_Array[currentObject.TypeID];
					if (coords.x >= (button.box.x + calibrateX) &&
						coords.x <= ((button.box.width + button.box.x) + calibrateX) &&
						coords.y >= (button.box.y + calibrateY) &&
						coords.y <= ((button.box.height + button.box.y) + calibrateY)) {
							currentObject.CallBack(i);
					}
				}
				break;
				case Label:
				break;
				case Frame:
				break;
				case CheckBox:
				break;
				case Dropdown:
				break;
			}
		}
	}
}

