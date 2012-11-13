/*
 * gui.h
 *
 *  Created on: Apr 29, 2012
 *      Author: matcat
 *      GUI Handling
 */

#ifndef GUI_H_
#define GUI_H_

#include "_gui.h"

#define RGB888_to_RGB565(r, g, b) (((r & 0xf8) << 8) | ((g & 0xfc) << 3) | ((b & 0xf8) >>3))
enum DialogType {
    dtOk, dtOkCancel, dtNone,
};

enum BarGraphDirection {
    BAR_HORIZONTAL,
    BAR_VERTICAL,
    TRIM_HORIZONTAL,
    TRIM_INVHORIZONTAL,
    TRIM_VERTICAL,
};
enum TextSelectType {
    TEXTSELECT_128,
    TEXTSELECT_64,
    TEXTSELECT_96,
    TEXTSELECT_DEVO10, // indication for textsel specically in devo10
};

enum KeyboardType {
    KEYBOARD_ALPHA,
    KEYBOARD_NUM,
    KEYBOARD_SPECIAL,
};

enum LabelType {
    LABEL_NO_BOX,
    LABEL_CENTER,
    LABEL_FILL,
    LABEL_TRANSPARENT,
    LABEL_UNDERLINE,
    LABEL_LEFT,    // align left and top vertically
    LABEL_INVERTED,
    LABEL_LEFTCENTER, // align left and center vertically
    LABEL_BOX,
    LABEL_BRACKET,
    LABEL_BLINK,
    LABEL_SQUAREBOX,
};

enum ListBoxType {
    LISTBOX_DEVO10,
    LISTBOX_OTHERS,
};

enum ListBoxNavigateKeyType {
    LISTBOX_KEY_UPDOWN,
    LISTBOX_KEY_RIGHTLEFT,
};

struct LabelDesc {
    u8 font;
    u16 font_color;
    u16 fill_color;
    u16 outline_color;
    enum LabelType style;
};

struct ImageMap {
    const char *file;
    u8 width;
    u8 height;
    u16 x_off;
    u16 y_off;
};
#ifndef ENABLE_GUIOBJECT
typedef void guiObject_t;
#else
typedef struct guiObject guiObject_t;

#include "buttons.h"

const struct ImageMap image_map[IMAGE_MAP_END];

enum GUIType {
    UnknownGUI,
    Button,
    Label,
    Image,
    CheckBox,
    Dropdown,
    Dialog,
    XYGraph,
    BarGraph,
    TextSelect,
    Listbox,
    Keyboard,
    Scrollbar,
    Rect,
};
struct guiImage {
    const char *file;
    u16 x_off;
    u16 y_off;
    u32 crc;
    void (*callback)(struct guiObject *obj, s8 press_type, const void *data);
    const void *cb_data;
};

struct guiBox {
    u16 x;
    u16 y;
    u16 width;
    u16 height;
};

struct guiLabel {
    struct LabelDesc desc;
    const char *(*strCallback)(struct guiObject *obj, const void *data);
    void (*pressCallback)(struct guiObject *obj, s8 press_type, const void *data);
    const void *cb_data;
};

struct guiKeyboard {
    u8 last_row;
    u8 last_col;
    u8 lastchar;
    char *text;
    s32 max_size;
    u8 flags;
    enum KeyboardType type;
    buttonAction_t action;
    void (*CallBack)(struct guiObject *obj, void *data);
    void *cb_data;
};

struct guiScrollbar {
    u8 state;
    u8 num_items;
    u8 cur_pos;
    buttonAction_t action;
    struct guiObject *parent;
    u8 (*callback)(struct guiObject *obj, u8 pos, s8 dir, void *data);
    void *cb_data;
};

struct guiButton {
    struct LabelDesc desc;
    const struct ImageMap *image;
    u16 fontColor;
    const char *(*strCallback)(struct guiObject *obj, const void *data);
    void (*CallBack)(struct guiObject *obj, const void *data);
    const void *cb_data;
    u8 enable;
};

struct guiListbox {
    struct LabelDesc desc;
    u8 text_height;
    u8 entries_per_page;
    u8 item_count;
    u8 cur_pos;
    s16 selected;
    struct guiObject *scrollbar;
    const char * (*string_cb)(u8 idx, void * data);
    void (*select_cb)(struct guiObject *obj, u16 selected, void * data);
    void (*longpress_cb)(struct guiObject *obj, u16 selected, void * data);
    void *cb_data;
    enum ListBoxType style;
    enum ListBoxNavigateKeyType key_style;
    buttonAction_t action; // fix bug for issue #81: DEVO10: Model list should be browsable with UP/DOWN
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
};

struct guiBarGraph {
    s16 min;
    s16 max;
    u8 direction;
    s16 (*CallBack)(void * data);
    void *cb_data;
};

struct guiDialog {
    struct guiBox txtbox;
    const char *title;
    const char *(*string_cb)(guiObject_t *obj, void *data);
    void (*CallBack)(u8 state, void *data);
    void *cbData;
};

struct guiTextSelect {
    const struct ImageMap *button;
    u8 state;
    enum TextSelectType type;
    u16 fontColor;
    const char *(*ValueCB)(guiObject_t *obj, int dir, void *data);
    void (*SelectCB)(guiObject_t *obj, void *data);
    void *cb_data;
    struct LabelDesc desc;
    u8 enable;
};

struct guiRect {
    struct LabelDesc desc;
};

struct guiObject {
    enum GUIType Type;
    struct guiBox box;
    u8 flags;
    struct guiObject *next;
    union {
        struct guiImage image;
        struct guiLabel label;
        struct guiButton button;
        struct guiXYGraph xy;
        struct guiBarGraph bar;
        struct guiDialog   dialog;
        struct guiTextSelect textselect;
        struct guiListbox listbox;
        struct guiKeyboard keyboard;
        struct guiScrollbar scrollbar;
        struct guiRect rect;
    } o;
};

#define OBJ_IS_USED(x)        ((x)->flags & 0x01) /* bool: UI element is in use */
#define OBJ_IS_HIDDEN(x)      ((x)->flags & 0x02) /* bool: UI element is not visible */
#define OBJ_IS_MODAL(x)       ((x)->flags & 0x04) /* bool: UI element is active and all non-model elements are not */
#define OBJ_IS_DIRTY(x)       ((x)->flags & 0x08) /* bool: UI element needs redraw */
#define OBJ_IS_TRANSPARENT(x) ((x)->flags & 0x10) /* bool: UI element has transparency */
#define OBJ_IS_SELECTABLE(x)  ((x)->flags & 0x20) /* bool: UI element can be selected */
#define OBJ_SET_USED(x,y)        (x)->flags = (y) ? (x)->flags | 0x01 : (x)->flags & ~0x01
#define OBJ_SET_HIDDEN(x,y)      (x)->flags = (y) ? (x)->flags | 0x02 : (x)->flags & ~0x02
#define OBJ_SET_MODAL(x,y)       (x)->flags = (y) ? (x)->flags | 0x04 : (x)->flags & ~0x04
#define OBJ_SET_DIRTY(x,y)       (x)->flags = (y) ? (x)->flags | 0x08 : (x)->flags & ~0x08
#define OBJ_SET_TRANSPARENT(x,y) (x)->flags = (y) ? (x)->flags | 0x10 : (x)->flags & ~0x10
#define OBJ_SET_SELECTABLE(x,y)  (x)->flags = (y) ? (x)->flags | 0x20 : (x)->flags & ~0x20

#define DRAW_NORMAL  0
#define DRAW_PRESSED 1

/* internal use only */
extern struct guiObject *objHEAD;
extern struct guiObject *objTOUCHED;
extern struct guiObject *objSELECTED;
extern struct guiObject *objModalButton;

void GUI_DrawKeyboard(struct guiObject *obj);
u8 GUI_TouchKeyboard(struct guiObject *obj, struct touch *coords, s8 press_type);

void GUI_DrawTextSelect(struct guiObject *obj);
u8 GUI_TouchTextSelect(struct guiObject *obj, struct touch *coords, s8 press_type);
void GUI_PressTextSelect(struct guiObject *obj, u32 button, u8 press_type);

void GUI_DrawXYGraph(struct guiObject *obj);
u8 GUI_TouchXYGraph(struct guiObject *obj, struct touch *coords, u8 long_press);

void GUI_DrawListbox(struct guiObject *obj, u8 redraw_all);
u8 GUI_TouchListbox(struct guiObject *obj, struct touch *coords, u8 long_press);
void GUI_PressListbox(struct guiObject *obj, u32 button, u8 press_type);

void GUI_DrawLabel(struct guiObject *obj);
u8 GUI_TouchLabel(struct guiObject *obj, struct touch *coords, s8 press_type);

void GUI_DrawDialog(struct guiObject *obj);
void GUI_DialogDrawBackground(u16 x, u16 y, u16 w, u16 h);

void GUI_DrawImage(struct guiObject *obj);
u8 GUI_TouchImage(struct guiObject *obj, struct touch *coords, s8 press_type);

void GUI_DrawButton(struct guiObject *obj);
void GUI_DrawBarGraph(struct guiObject *obj);
void GUI_DrawScrollbar(struct guiObject *obj);
u8 GUI_TouchScrollbar(struct guiObject *obj, struct touch *coords, s8 press_type);

void GUI_DrawRect(struct guiObject *obj);

void GUI_DrawObject(struct guiObject *obj);
void GUI_DrawBackground(u16 x, u16 y, u16 w, u16 h);
void GUI_DrawImageHelper(u16 x, u16 y, const struct ImageMap *map, u8 idx);
u8 coords_in_box(struct guiBox *box, struct touch *coords);
struct guiObject *GUI_GetFreeObj(void);
void connect_object(struct guiObject *obj);
void GUI_HandleModalButtons(u8 enable);
int GUI_ButtonWidth(enum ButtonType type);
int GUI_ButtonHeight(enum ButtonType type);
#endif

guiObject_t *GUI_CreateDialog(u16 x, u16 y, u16 width, u16 height, const char *title,
        const char *(*string_cb)(guiObject_t *obj, void *data),
        void (*CallBack)(u8 state, void *data),
        enum DialogType dgType, void *data);
#define GUI_CreateLabel(x, y, cb, desc, data) GUI_CreateLabelBox(x, y, 0, 0, &desc, cb, NULL, data)
guiObject_t *GUI_CreateLabelBox(u16 x, u16 y, u16 width, u16 height, const struct LabelDesc *desc,
             const char *(*strCallback)(guiObject_t *, const void *),
             void (*pressCallback)(guiObject_t *obj, s8 press_type, const void *data),
             const void *data);
void GUI_DrawLabelHelper(u16 obj_x, u16 obj_y, u16 obj_width, u16 obj_height,
        const char *str, const struct LabelDesc *desc, u8 is_selected);
void GUI_SetLabelDesc(guiObject_t *obj, struct LabelDesc *desc);

#define GUI_CreateImage(x, y, w,h, file) GUI_CreateImageOffset(x, y, w, h, 0, 0, file, NULL, NULL)
guiObject_t *GUI_CreateImageOffset(u16 x, u16 y, u16 width, u16 height, u16 x_off, u16 y_off, const char *file,
        void (*CallBack)(guiObject_t *obj, s8 press_type, const void *data), const void *cb_data);

guiObject_t *GUI_CreateButton(u16 x, u16 y, enum ButtonType type,
        const char *(*strCallback)(guiObject_t *, const void *),
        u16 fontColor, void (*CallBack)(guiObject_t *obj, const void *data), const void *cb_data);
guiObject_t *GUI_CreateButtonPlateText(u16 x, u16 y, u16 width, u16 height, const struct LabelDesc *desc,
        const char *(*strCallback)(guiObject_t *, const void *),
        u16 fontColor, void (*CallBack)(guiObject_t *obj, const void *data), const void *cb_data);
guiObject_t *GUI_CreateIcon(u16 x, u16 y, const struct ImageMap *image,
        void (*CallBack)(guiObject_t *obj, const void *data), const void *cb_data);
void GUI_ButtonEnable(guiObject_t *obj, u8 enable);
u8 GUI_IsButtonEnabled(guiObject_t *obj);

guiObject_t *GUI_CreateListBox(u16 x, u16 y, u16 width, u16 height, u8 item_count, s16 selected,
        const char *(*string_cb)(u8 idx, void *data),
        void (*select_cb)(guiObject_t *obj, u16 selected, void *data),
        void (*longpress_cb)(guiObject_t *obj, u16 selected, void *data),
        void *cb_data);
guiObject_t *GUI_CreateListBoxPlateText(u16 x, u16 y, u16 width, u16 height, u8 item_count, s16 selected,
        const struct LabelDesc *desc, enum ListBoxNavigateKeyType keyType,
        const char *(*string_cb)(u8 idx, void *data),
        void (*select_cb)(guiObject_t *obj, u16 selected, void *data),
        void (*longpress_cb)(guiObject_t *obj, u16 selected, void *data),
        void *cb_data);
void GUI_ListBoxSelect(guiObject_t *obj, u16 selected);

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
guiObject_t *GUI_CreateTextSelectPlate(u16 x, u16 y, u16 width, u16 height, const struct LabelDesc *desc,
        void (*select_cb)(guiObject_t *obj, void *data),
        const char *(*value_cb)(guiObject_t *obj, int value, void *data),
        void *cb_data);
guiObject_t *GUI_CreateKeyboard(enum KeyboardType type, char *text, s32 max_size,
        void (*CallBack)(guiObject_t *obj, void *data), void *cb_data);

guiObject_t *GUI_CreateScrollbar(u16 x, u16 y, u16 height,
        u8 num_items, guiObject_t *parent,
        u8 (*press_cb)(guiObject_t *parent, u8 pos, s8 direction, void *data), void *data);
void GUI_SetScrollbar(guiObject_t *obj, u8 pos);

guiObject_t *GUI_CreateRect(u16 x, u16 y, u16 width, u16 height, const struct LabelDesc *desc);

u8 GUI_CheckTouch(struct touch *coords, u8 long_press);
void GUI_TouchRelease();
void GUI_DrawScreen(void);
void GUI_RefreshScreen(void);
void GUI_Redraw(guiObject_t *obj);
void GUI_RedrawAllObjects();
void GUI_RemoveObj(guiObject_t *obj);
void GUI_RemoveAllObjects();
void GUI_RemoveHierObjects(guiObject_t *obj);
void GUI_DrawObjects(void);
void GUI_SetHidden(guiObject_t *obj, u8 state);
guiObject_t *GUI_GetSelected();
u8 GUI_IsSelectable(guiObject_t *obj);
void GUI_SetSelected(guiObject_t *obj);
void GUI_SetSelectable(guiObject_t *obj, u8 selectable);
u8 GUI_ObjectNeedsRedraw(guiObject_t *obj);
guiObject_t *GUI_IsModal(void);
void GUI_HandleButtons(u8 enable);
struct guiObject *GUI_GetNextSelectable(struct guiObject *origObj);
struct guiObject *GUI_GetPrevSelectable(struct guiObject *origObj);
void GUI_GetSize(guiObject_t *obj, int *width, int *height);

s32 GUI_TextSelectHelper(s32 value, s32 min, s32 max, s8 dir, u32 shortstep, u32 longstep, u8 *_changed);
void GUI_TextSelectEnablePress(guiObject_t *obj, u8 enable);
void GUI_TextSelectEnable(struct guiObject *obj, u8 enable);
u8 GUI_IsTextSelectEnabled(struct guiObject *obj);
void GUI_ChangeImage(guiObject_t *obj, const char *file, u16 x_off, u16 y_off);

#define LOGICAL_VIEW_BOUNDARY 5000 // a coordinate that is >= 5000 is a relative coordinate
#define GUI_IsLogicViewCoordinate(x) (x)>= LOGICAL_VIEW_BOUNDARY
u8 GUI_IsObjectInsideCurrentView(u8 view_id, struct guiObject *obj);
u8 GUI_IsCoordinateInsideLogicalView(u8 view_id, u16 *x, u16 *y);
void GUI_SetupLogicalView(u8 view_id, u16 view_orgin_relativeX, u16 view_orgin_relativeY, u8 width, u8 height,
        u8 view_origin_absoluteX, u8 view_origin_absoluteY);
u16 GUI_MapToLogicalView(u8 view_id, u16 x_or_y);
void GUI_SetRelativeOrigin(u8 view_id, s16 new_originX, s16 new_originY);
void GUI_ScrollLogicalView(u8 view_id, s16 y_offset);
void GUI_ScrollLogicalViewToObject(u8 view_id, struct guiObject *obj, s8 direction);
s16 GUI_GetLogicalViewOriginRelativeY(u8 view_id);
s8 GUI_GetViewId(u16 x, u16 y) ;
#endif /* GUI_H_ */
