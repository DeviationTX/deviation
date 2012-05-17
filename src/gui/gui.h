/*
 * gui.h
 *
 *  Created on: Apr 29, 2012
 *      Author: matcat
 *      GUI Handling
 */

#ifndef GUI_H_
#define GUI_H_

enum DialogType {
    dtOk, dtOkCancel
};

enum BarGraphDirection {
    BAR_HORIZONTAL,
    BAR_VERTICAL
};

enum GUIType {
    UnknownGUI,
    Button,
    Label,
    Frame,
    CheckBox,
    Dropdown,
    Dialog,
    XYGraph,
    BarGraph,
    TextSelect,
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
    u8 inuse;
};
struct guiFrame {
    u8 inuse;
};
struct guiButton {
    const char *text;
    u16 text_x_off;
    u16 text_y_off;
    u16 fontColor;
    u8 inuse;
};

struct guiXYGraph {
    s16 min_x;
    s16 min_y;
    s16 max_x;
    s16 max_y;
    s16 (*CallBack)(s16 xval, void * data);
    void *cb_data;
    u8 inuse;
};

struct guiBarGraph {
    s16 min;
    s16 max;
    u8 direction;
    s16 (*CallBack)(void * data);
    void *cb_data;
    u8 inuse;
};

struct guiDialogReturn {
    u8 buttonPushed;
    char strInput[80];
    int intInput;
};

struct guiDialog {
    const char *text;
    const char *title;
    enum DialogType Type;
    u16 fontColor;
    u16 titleColor;
    int buttonid[4];
    void (*CallBack)(int ObjID, struct guiDialogReturn gDR);
    u8 inuse;
};

struct guiTextSelect {
    u16 fontColor;
    const char *(*ValueCB)(int ObjID, int dir, void *data);
    void (*SelectCB)(int ObjID, void *data);
    void *cb_data;
    u8 inuse;
};
#define OBJ_IS_DISABLED(x)    ((x).flags & 0x01) /* bool: UI element is not 'active' */
#define OBJ_IS_MODAL(x)       ((x).flags & 0x02) /* bool: UI element is active and all non-model elements are not */
#define OBJ_IS_DIRTY(x)       ((x).flags & 0x04) /* bool: UI element needs redraw */
#define OBJ_IS_TRANSPARENT(x) ((x).flags & 0x08) /* bool: UI element has transparency */
#define OBJ_SET_DISABLED(x,y)    (x).flags = y ? (x).flags | 0x01 : (x).flags & ~0x01
#define OBJ_SET_MODAL(x,y)       (x).flags = y ? (x).flags | 0x02 : (x).flags & ~0x02
#define OBJ_SET_DIRTY(x,y)       (x).flags = y ? (x).flags | 0x04 : (x).flags & ~0x04
#define OBJ_SET_TRANSPARENT(x,y) (x).flags = y ? (x).flags | 0x08 : (x).flags & ~0x08
struct guiObject {
    enum GUIType Type;
    void (*CallBack)(int ObjID);
    struct guiBox box;
    int GUIID;
    int TypeID;
    int parent;
    u8 flags;
};
u8 GUI_CheckModel(void);
int GUI_CreateDialog(u16 x, u16 y, u16 width, u16 height, const char *title,
        const char *text, u16 titleColor, u16 fontColor,
        void (*CallBack)(int ObjID, struct guiDialogReturn),
        enum DialogType dgType);
int GUI_CreateLabel(u16 x, u16 y, const char *text, u16 fontColor);
int GUI_CreateFrame(u16 x, u16 y, u16 width, u16 height, const char *image);
int GUI_CreateButton(u16 x, u16 y, u16 width, u16 height, const char *text,
        u16 fontColor, void (*CallBack)(int ObjID));
int GUI_CreateXYGraph(u16 x, u16 y, u16 width, u16 height, s16 min_x,
        s16 min_y, s16 max_x, s16 max_y, s16 (*Callback)(s16 xval, void * data), void * cb_data);
int GUI_CreateBarGraph(u16 x, u16 y, u16 width, u16 height, s16 min,
        s16 max, u8 direction, s16 (*Callback)(void * data), void * cb_data);
int GUI_CreateTextSelect(u16 x, u16 y, u16 width, u16 height, u16 fontColor,
        void (*select_cb)(int ObjID, void *data),
        const char *(*value_cb)(int ObjID, int value, void *data),
        void *cb_data);
u8 GUI_CheckTouch(struct touch coords);
void GUI_DrawScreen(void);
void GUI_RefreshScreen(void);
void GUI_Redraw(u8 ObjID);
void GUI_DrawObject(int ObjID);
void GUI_DrawObjects(void);
void GUI_RemoveObj(int objID);
int GUI_GetFreeObj(void);
int GUI_GetFreeGUIObj(enum GUIType guiType);
void dgCallback(int ObjID);
void GUI_DrawWindow(int ObjID);
#endif /* GUI_H_ */
