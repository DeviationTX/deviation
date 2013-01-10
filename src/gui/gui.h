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
    TEXTSELECT_224,
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
    LABEL_SQUAREBOX,
    LABEL_INVERTED,
    LABEL_LEFT,    // align left and top vertically
    LABEL_LEFTCENTER, // align left and center vertically
    LABEL_BOX,
    LABEL_BRACKET,
    LABEL_BLINK,
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

#include "buttons.h"

extern const struct ImageMap image_map[IMAGE_MAP_END];

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

struct guiBox {
    u16 x;
    u16 y;
    u16 width;
    u16 height;
};

#define guiObject guiHeader
struct guiHeader {
    struct guiBox box;
    struct guiObject *next;
    enum GUIType Type;
    u8 flags;
};
typedef struct guiHeader guiObject_t;



typedef struct guiImage {
    struct guiHeader header;
    const char *file;
    u16 x_off;
    u16 y_off;
    u32 crc;
    void (*callback)(struct guiObject *obj, s8 press_type, const void *data);
    const void *cb_data;
} guiImage_t;

typedef struct guiLabel {
    struct guiHeader header;
    struct LabelDesc desc;
    const char *(*strCallback)(struct guiObject *obj, const void *data);
    void (*pressCallback)(struct guiObject *obj, s8 press_type, const void *data);
    const void *cb_data;
} guiLabel_t;

typedef struct guiKeyboard {
    struct guiHeader header;
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
} guiKeyboard_t;

typedef struct guiScrollbar {
    struct guiHeader header;
    u8 state;
    u8 num_items;
    u8 cur_pos;
    buttonAction_t action;
    struct guiObject *parent;
    u8 (*callback)(struct guiObject *obj, u8 pos, s8 dir, void *data);
    void *cb_data;
} guiScrollbar_t;

typedef struct guiButton {
    struct guiHeader header;
    struct LabelDesc desc;
    const struct ImageMap *image;
    u16 fontColor;
    const char *(*strCallback)(struct guiObject *obj, const void *data);
    void (*CallBack)(struct guiObject *obj, const void *data);
    const void *cb_data;
    u8 enable;
} guiButton_t;

typedef struct guiListbox {
    struct guiHeader header;
    struct LabelDesc desc;
    u8 text_height;
    u8 entries_per_page;
    u8 item_count;
    u8 cur_pos;
    s16 selected;
    struct guiScrollbar scrollbar;
    const char * (*string_cb)(u8 idx, void * data);
    void (*select_cb)(struct guiObject *obj, u16 selected, void * data);
    void (*longpress_cb)(struct guiObject *obj, u16 selected, void * data);
    void *cb_data;
    enum ListBoxType style;
    enum ListBoxNavigateKeyType key_style;
    buttonAction_t action; // fix bug for issue #81: DEVO10: Model list should be browsable with UP/DOWN
} guiListbox_t;

typedef struct guiXYGraph {
    struct guiHeader header;
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
} guiXYGraph_t;

typedef struct guiBarGraph {
    struct guiHeader header;
    s16 min;
    s16 max;
    u8 direction;
    s16 (*CallBack)(void * data);
    void *cb_data;
} guiBarGraph_t;

typedef struct guiDialog {
    struct guiHeader header;
    struct guiBox txtbox;
    guiButton_t but1;
    guiButton_t but2;
    const char *title;
    const char *(*string_cb)(guiObject_t *obj, void *data);
    void (*CallBack)(u8 state, void *data);
    void *cbData;
} guiDialog_t;

typedef struct guiTextSelect {
    struct guiHeader header;
    const struct ImageMap *button;
    u8 state;
    enum TextSelectType type;
    u16 fontColor;
    const char *(*ValueCB)(guiObject_t *obj, int dir, void *data);
    void (*SelectCB)(guiObject_t *obj, void *data);
    void *cb_data;
    struct LabelDesc desc;
    u8 enable;
} guiTextSelect_t;

typedef struct guiRect {
    struct guiHeader header;
    struct LabelDesc desc;
} guiRect_t;

#define OBJ_IS_USED(x)        (((guiObject_t *)(x))->flags & 0x01) /* bool: UI element is in use */
#define OBJ_IS_HIDDEN(x)      ((x)->flags & 0x02) /* bool: UI element is not visible */
#define OBJ_IS_MODAL(x)       ((x)->flags & 0x04) /* bool: UI element is active and all non-model elements are not */
#define OBJ_IS_DIRTY(x)       ((x)->flags & 0x08) /* bool: UI element needs redraw */
#define OBJ_IS_TRANSPARENT(x) ((x)->flags & 0x10) /* bool: UI element has transparency */
#define OBJ_IS_SELECTABLE(x)  ((x)->flags & 0x20) /* bool: UI element can be selected */
#define OBJ_SET_FLAG(obj,flag,set)  ((guiObject_t *)(obj))->flags = (set) \
                                    ? ((guiObject_t *)(obj))->flags | (flag) \
                                    : ((guiObject_t *)(obj))->flags & ~(flag)
#define OBJ_SET_USED(x,y)        OBJ_SET_FLAG(x, 0x01, y)
#define OBJ_SET_HIDDEN(x,y)      OBJ_SET_FLAG(x, 0x02, y)
#define OBJ_SET_MODAL(x,y)       OBJ_SET_FLAG(x, 0x04, y)
#define OBJ_SET_DIRTY(x,y)       OBJ_SET_FLAG(x, 0x08, y)
#define OBJ_SET_TRANSPARENT(x,y) OBJ_SET_FLAG(x, 0x10, y)
#define OBJ_SET_SELECTABLE(x,y)  OBJ_SET_FLAG(x, 0x20, y)

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
void connect_object(struct guiObject *obj);
void GUI_HandleModalButtons(u8 enable);
int GUI_ButtonWidth(enum ButtonType type);
int GUI_ButtonHeight(enum ButtonType type);

guiObject_t *GUI_CreateDialog(guiDialog_t *,u16 x, u16 y, u16 width, u16 height, const char *title,
        const char *(*string_cb)(guiObject_t *obj, void *data),
        void (*CallBack)(u8 state, void *data),
        enum DialogType dgType, void *data);
#define GUI_CreateLabel(obj, x, y, cb, desc, data) GUI_CreateLabelBox(obj, x, y, 0, 0, &desc, cb, NULL, data)
guiObject_t *GUI_CreateLabelBox(guiLabel_t *,u16 x, u16 y, u16 width, u16 height, const struct LabelDesc *desc,
             const char *(*strCallback)(guiObject_t *, const void *),
             void (*pressCallback)(guiObject_t *obj, s8 press_type, const void *data),
             const void *data);
void GUI_DrawLabelHelper(u16 obj_x, u16 obj_y, u16 obj_width, u16 obj_height,
        const char *str, const struct LabelDesc *desc, u8 is_selected);
void GUI_SetLabelDesc(guiLabel_t *obj, struct LabelDesc *desc);

#define GUI_CreateImage(obj, x, y, w,h, file) GUI_CreateImageOffset(obj, x, y, w, h, 0, 0, file, NULL, NULL)
guiObject_t *GUI_CreateImageOffset(guiImage_t *, u16 x, u16 y, u16 width, u16 height, u16 x_off, u16 y_off, const char *file,
        void (*CallBack)(guiObject_t *obj, s8 press_type, const void *data), const void *cb_data);

guiObject_t *GUI_CreateButton(guiButton_t *, u16 x, u16 y, enum ButtonType type,
        const char *(*strCallback)(guiObject_t *, const void *),
        u16 fontColor, void (*CallBack)(guiObject_t *obj, const void *data), const void *cb_data);
guiObject_t *GUI_CreateButtonPlateText(guiButton_t *, u16 x, u16 y, u16 width, u16 height, const struct LabelDesc *desc,
        const char *(*strCallback)(guiObject_t *, const void *),
        u16 fontColor, void (*CallBack)(guiObject_t *obj, const void *data), const void *cb_data);
guiObject_t *GUI_CreateIcon(guiButton_t *, u16 x, u16 y, const struct ImageMap *image,
        void (*CallBack)(guiObject_t *obj, const void *data), const void *cb_data);
void GUI_ButtonEnable(guiObject_t *obj, u8 enable);
u8 GUI_IsButtonEnabled(guiObject_t *obj);

guiObject_t *GUI_CreateListBox(guiListbox_t *, u16 x, u16 y, u16 width, u16 height, u8 item_count, s16 selected,
        const char *(*string_cb)(u8 idx, void *data),
        void (*select_cb)(guiObject_t *obj, u16 selected, void *data),
        void (*longpress_cb)(guiObject_t *obj, u16 selected, void *data),
        void *cb_data);
guiObject_t *GUI_CreateListBoxPlateText(guiListbox_t *, u16 x, u16 y, u16 width, u16 height, u8 item_count, s16 selected,
        const struct LabelDesc *desc, enum ListBoxNavigateKeyType keyType,
        const char *(*string_cb)(u8 idx, void *data),
        void (*select_cb)(guiObject_t *obj, u16 selected, void *data),
        void (*longpress_cb)(guiObject_t *obj, u16 selected, void *data),
        void *cb_data);
void GUI_ListBoxSelect(guiListbox_t *obj, u16 selected);

guiObject_t *GUI_CreateXYGraph(guiXYGraph_t *, u16 x, u16 y, u16 width, u16 height,
                      s16 min_x, s16 min_y, s16 max_x, s16 max_y,
                      u16 gridx, u16 gridy,
                      s16 (*Callback)(s16 xval, void *data), 
                      u8 (*point_cb)(s16 *x, s16 *y, u8 pos, void *data),
                      u8 (*touch_cb)(s16 x, s16 y, void *data),
                      void *cb_data);
guiObject_t *GUI_CreateBarGraph(guiBarGraph_t *, u16 x, u16 y, u16 width, u16 height, s16 min,
        s16 max, u8 direction, s16 (*Callback)(void * data), void * cb_data);
guiObject_t *GUI_CreateTextSelect(guiTextSelect_t *, u16 x, u16 y, enum TextSelectType type, u16 fontColor,
        void (*select_cb)(guiObject_t *obj, void *data),
        const char *(*value_cb)(guiObject_t *obj, int value, void *data),
        void *cb_data);
guiObject_t *GUI_CreateTextSelectPlate(guiTextSelect_t *, u16 x, u16 y, u16 width, u16 height, const struct LabelDesc *desc,
        void (*select_cb)(guiObject_t *obj, void *data),
        const char *(*value_cb)(guiObject_t *obj, int value, void *data),
        void *cb_data);
guiObject_t *GUI_CreateKeyboard(guiKeyboard_t *, enum KeyboardType type, char *text, s32 max_size,
        void (*CallBack)(guiObject_t *obj, void *data), void *cb_data);

guiObject_t *GUI_CreateScrollbar(guiScrollbar_t *, u16 x, u16 y, u16 height,
        u8 num_items, guiObject_t *parent,
        u8 (*press_cb)(guiObject_t *parent, u8 pos, s8 direction, void *data), void *data);
void GUI_SetScrollbar(guiScrollbar_t *obj, u8 pos);
u8 GUI_GetScrollbarNumItems(guiScrollbar_t *obj);

guiObject_t *GUI_CreateRect(guiRect_t *, u16 x, u16 y, u16 width, u16 height, const struct LabelDesc *desc);

u8 GUI_CheckTouch(struct touch *coords, u8 long_press);
void GUI_TouchRelease();
void GUI_DrawScreen(void);
void GUI_RefreshScreen(void);
void _GUI_Redraw(guiObject_t *obj);
#define GUI_Redraw(x) _GUI_Redraw((guiObject_t *)(x))
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
void GUI_TextSelectEnablePress(guiTextSelect_t *obj, u8 enable);
void GUI_TextSelectEnable(guiTextSelect_t *obj, u8 enable);
u8 GUI_IsTextSelectEnabled(struct guiObject *obj);
void GUI_ChangeImage(guiImage_t *obj, const char *file, u16 x_off, u16 y_off);

// logical view, only available in text-based LCD, such as devo10
struct viewObject {
    /*This is a hack to allow use of OBJ_* macros */
    /*This provides the same layout as guiHeader */
    u16 origin_absoluteX;
    u16 origin_absoluteY;
    u16 width;
    u16 height;
    struct viewObject *next;
    enum GUIType Type;
    u8 flags;
    /*end*/
    s16 orgin_relativeX;  // can be negative to indicate the whole view is hidden
    s16 orgin_relativeY; // can be negative to indicate the whole view is hidden
    u16 boundary;
};
typedef struct viewObject viewObject_t;

#define LOGICALVIEW_COUNT 3
#define LOGICAL_VIEW_BOUNDARY 5000 // a coordinate that is >= 5000 is a relative coordinate
#define GUI_IsLogicViewCoordinate(x) (x)>= LOGICAL_VIEW_BOUNDARY
u8 GUI_IsObjectInsideCurrentView(u8 view_id, struct guiObject *obj);
u8 GUI_IsCoordinateInsideLogicalView(u16 *x, u16 *y);
void GUI_SetupLogicalView(u8 view_id, u16 view_orgin_relativeX, u16 view_orgin_relativeY, u8 width, u8 height,
        u8 view_origin_absoluteX, u8 view_origin_absoluteY);
u16 GUI_MapToLogicalView(u8 view_id, u16 x_or_y);
void GUI_SetRelativeOrigin(u8 view_id, s16 new_originX, s16 new_originY);
void GUI_ScrollLogicalView(u8 view_id, s16 y_offset);
void GUI_ScrollLogicalViewToObject(u8 view_id, struct guiObject *obj, s8 direction);
s16 GUI_GetLogicalViewOriginRelativeY(u8 view_id);
void GUI_Select1stSelectableObj();
s8 GUI_GetViewId(s16 x, s16 y);
void GUI_ViewInit();
#endif /* GUI_H_ */
