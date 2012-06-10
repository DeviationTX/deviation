/*
 * gui.h
 *
 *  Created on: Apr 29, 2012
 *      Author: matcat
 *      GUI Handling
 */

#ifndef GUI_H_
#define GUI_H_

#define RGB888_to_RGB565(r, g, b) (((r & 0xf8) << 8) | ((g & 0xfc) << 3) | ((b & 0xf8) >>3))
enum DialogType {
    dtOk, dtOkCancel
};

enum BarGraphDirection {
    BAR_HORIZONTAL,
    BAR_VERTICAL,
    TRIM_HORIZONTAL,
    TRIM_VERTICAL,
};
enum TextSelectType {
    TEXTSELECT_128,
    TEXTSELECT_64,
    TEXTSELECT_96,
};

enum ButtonType {
    BUTTON_90,
    BUTTON_45,
    BUTTON_96x16,
    BUTTON_64x16,
    BUTTON_48x16,
    BUTTON_32x16,
};

struct guiDialogReturn {
    u8 buttonPushed;
    char strInput[80];
    int intInput;
};

#ifndef ENABLE_GUIOBJECT
typedef void guiObject_t;
#else
typedef struct guiObject guiObject_t;

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

struct guiObject {
    enum GUIType Type;
    struct guiBox box;
    u8 flags;
    void *widget;
    struct guiObject *next;
    struct guiObject *parent;
};

struct guiLabel {
    const char *(*CallBack)(struct guiObject *obj, void *data);
    void *cb_data;
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
    void (*CallBack)(struct guiObject *obj, void *data);
    void *cb_data;
    u8 inuse;
};

struct guiXYGraph {
    s16 min_x;
    s16 min_y;
    s16 max_x;
    s16 max_y;
    u16 grid_x;
    u16 grid_y;
    s16 (*CallBack)(s16 xval, void * data);
    u8 (*point_cb)(s16 *x, s16 *y, u8 pos, void *data);
    u8 (*touch_cb)(s16 x, s16 y, void *data);
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

struct guiDialog {
    const char *text;
    const char *title;
    enum DialogType Type;
    u16 fontColor;
    u16 titleColor;
    struct guiObject *button[4];
    void (*CallBack)(guiObject_t *obj, struct guiDialogReturn gDR);
    u8 inuse;
};

struct guiTextSelect {
    u16 fontColor;
    const char *(*ValueCB)(guiObject_t *obj, int dir, void *data);
    void (*SelectCB)(guiObject_t *obj, void *data);
    void *cb_data;
    u8 inuse;
};
#define OBJ_IS_USED(x)        ((x)->flags & 0x01) /* bool: UI element is in use */
#define OBJ_IS_DISABLED(x)    ((x)->flags & 0x02) /* bool: UI element is not 'active' */
#define OBJ_IS_MODAL(x)       ((x)->flags & 0x04) /* bool: UI element is active and all non-model elements are not */
#define OBJ_IS_DIRTY(x)       ((x)->flags & 0x08) /* bool: UI element needs redraw */
#define OBJ_IS_TRANSPARENT(x) ((x)->flags & 0x10) /* bool: UI element has transparency */
#define OBJ_SET_USED(x,y)        (x)->flags = y ? (x)->flags | 0x01 : (x)->flags & ~0x01
#define OBJ_SET_DISABLED(x,y)    (x)->flags = y ? (x)->flags | 0x02 : (x)->flags & ~0x02
#define OBJ_SET_MODAL(x,y)       (x)->flags = y ? (x)->flags | 0x04 : (x)->flags & ~0x04
#define OBJ_SET_DIRTY(x,y)       (x)->flags = y ? (x)->flags | 0x08 : (x)->flags & ~0x08
#define OBJ_SET_TRANSPARENT(x,y) (x)->flags = y ? (x)->flags | 0x10 : (x)->flags & ~0x10
#endif

guiObject_t *GUI_CreateDialog(u16 x, u16 y, u16 width, u16 height, const char *title,
        const char *text, u16 titleColor, u16 fontColor,
        void (*CallBack)(guiObject_t *obj, struct guiDialogReturn),
        enum DialogType dgType);
guiObject_t *GUI_CreateLabel(u16 x, u16 y, const char *(*Callback)(guiObject_t *obj, void *data), u16 fontColor, void *data);
guiObject_t *GUI_CreateFrame(u16 x, u16 y, u16 width, u16 height, const char *image);
guiObject_t *GUI_CreateButton(u16 x, u16 y, enum ButtonType type, const char *text,
        u16 fontColor, void (*CallBack)(guiObject_t *obj, void *data), void *cb_data);
guiObject_t *GUI_CreateXYGraph(u16 x, u16 y, u16 width, u16 height,
                      s16 min_x, s16 min_y, s16 max_x, s16 max_y,
                      u16 gridx, u16 gridy,
                      s16 (*Callback)(s16 xval, void *data), 
                      u8 (*point_cb)(s16 *x, s16 *y, u8 pos, void *data),
                      u8 (*touch_cb)(s16 x, s16 y, void *data),
                      void *cb_data);
guiObject_t *GUI_CreateBarGraph(u16 x, u16 y, u16 width, u16 height, s16 min,
        s16 max, u8 direction, s16 (*Callback)(void * data), void * cb_data);
guiObject_t *GUI_CreateTextSelect(u16 x, u16 y, enum TextSelectType type, u16 fontColor,
        void (*select_cb)(guiObject_t *obj, void *data),
        const char *(*value_cb)(guiObject_t *obj, int value, void *data),
        void *cb_data);
u8 GUI_CheckTouch(struct touch *coords, u8 long_press);
void GUI_DrawScreen(void);
void GUI_RefreshScreen(void);
void GUI_Redraw(guiObject_t *obj);
void GUI_DrawObjects(void);
void GUI_RemoveObj(guiObject_t *obj);
void GUI_RemoveAllObjects();
u8 GUI_IsModal(void);
s32 GUI_TextSelectHelper(s32 value, s32 min, s32 max, s8 dir, u8 shortstep, u8 longstep, u8 *_changed);
#endif /* GUI_H_ */
