/*
 * gui.h
 *
 *  Created on: Apr 29, 2012
 *      Author: matcat
 *      GUI Handling
 */

#ifndef GUI_H_
#define GUI_H_

enum GUIType {
	Button,
	Label,
	Frame,
	CheckBox,
	Dropdown
};
struct guiImage {
	const char *file;
	u16 x_off;
	u16 y_off;
};

struct guiBox {
	u16 x;
	u16 y;
	u16 width;
	u16 height;
	struct guiImage image;
};
struct guiLabel {
	const char *text;
	u16 fontColor;
	struct guiBox box;
};
struct guiFrame {
	struct guiBox box;
};
struct guiButton {
	const char *text;
	u16 fontColor;
	struct guiBox box;
};
struct guiObject {
	enum GUIType Type;
	void (*CallBack)(int ObjID);
	int GUIID;
	int TypeID;
	u8 Disabled; /* bool: Means this UI element is not 'active' */
	u8 Model;	/* bool: Means this UI element is active and all non-model elements are not */
};
u8 GUI_CheckModel(void);
char* GUI_GetText(int GUIID);
void GUI_SetText(int GUIID,const char *text);
int GUI_CreateLabel(u16 x, u16 y, const char *text, u16 fontColor);
int GUI_CreateFrame(u16 x, u16 y, u16 width, u16 height, const char *image);
int GUI_CreateButton(u16 x, u16 y, u16 width, u16 height, const char *text, u16 fontColor, void (*CallBack)(int ObjID));
void GUI_CheckTouch(struct touch coords);
void GUI_DrawScreen(void);
void GUI_DrawObjects(void);
void GUI_RemoveObj(int objID);
int GUI_GetFreeObj(void);
int GUI_GetFreeGUIObj(enum GUIType guiType);

#endif /* GUI_H_ */
