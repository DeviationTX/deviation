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
	CheckBox,
	Dropdown,
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

struct guiButton {
	char text[40];
	struct guiBox box;

};
struct guiObject {
	enum GUIType Type;
	void (*CallBack)(void);
	int GUIID;
};

int GUI_CreateButton(u16 x, u16 y, u16 width, u16 height, const char *text, void (*CallBack)(void));
void GUI_CheckTouch(struct touch coords);


#endif /* GUI_H_ */
