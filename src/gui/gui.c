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
#include "target.h"
#define ENABLE_GUIOBJECT
#include "gui.h"

struct guiObject GUI_Array[100];
struct guiObject *objHEAD = NULL;
static u8 FullRedraw;

static void GUI_DrawObject(struct guiObject *obj);
static void dgCallback(struct guiObject *obj, void *data);

static void GUI_DrawLabel(struct guiObject *obj);
static void GUI_DrawBarGraph(struct guiObject *obj);
static void GUI_DrawXYGraph(struct guiObject *obj);
static void GUI_DrawTextSelect(struct guiObject *obj);

static void GUI_DrawListbox(struct guiObject *obj, u8 redraw_all);
static u8 GUI_TouchListbox(struct guiObject *obj, struct touch *coords, u8 long_press);

#define ARROW_UP 0
#define ARROW_DOWN 16
#define ARROW_RIGHT 32
#define ARROW_LEFT 48
#define ARROW_WIDTH 16
#define ARROW_HEIGHT 16
const char ARROW_FILE[] = "images/arrows.bmp";

void connect_object(struct guiObject *obj)
{
    if (objHEAD == NULL) {
        objHEAD = obj;
    } else {
        struct guiObject *ptr = objHEAD;
        while(ptr->next)
            ptr = ptr->next;
        ptr->next = obj;
    }
}

guiObject_t *GUI_CreateDialog(u16 x, u16 y, u16 width, u16 height, const char *title,
        const char *text, u16 titleColor, u16 fontColor,
        void (*CallBack)(guiObject_t *obj, struct guiDialogReturn),
        enum DialogType dgType)
{
    struct guiObject *obj = GUI_GetFreeObj();
    struct guiBox *box ;
    struct guiDialog *dialog;

    if (obj == NULL)
        return NULL;

    box = &obj->box;
    dialog = &obj->o.dialog;

    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;
    dialog->image.file = "images/dialog.bmp";
    dialog->image.x_off = 0;
    dialog->image.y_off = 0;

    obj->Type = Dialog;
    OBJ_SET_TRANSPARENT(obj, LCD_ImageIsTransparent(dialog->image.file));
    OBJ_SET_MODAL(obj, 1);
    OBJ_SET_USED(obj, 1);
    connect_object(obj);

    dialog->text = text;
    dialog->title = title;
    dialog->fontColor = fontColor;
    dialog->titleColor = titleColor;
    dialog->Type = dgType;
    dialog->CallBack = *CallBack;

    dialog->button[0] = NULL;
    dialog->button[1] = NULL;
    dialog->button[2] = NULL;
    dialog->button[3] = NULL;

    switch (dgType) {
    case dtOk:
        dialog->button[0] = GUI_CreateButton(((x + width) / 2) - 10,
                ((y + height) - 27), BUTTON_90, "Ok", 0x0000, dgCallback, obj);
        OBJ_SET_MODAL(dialog->button[0], 1);
        break;
    case dtOkCancel:
        break;
    }

    return obj;

}

guiObject_t *GUI_CreateTextSelect(u16 x, u16 y, enum TextSelectType type, u16 fontColor,
        void (*select_cb)(guiObject_t *obj, void *data),
        const char *(*value_cb)(guiObject_t *obj, int value, void *data),
        void *cb_data)
{
    struct guiObject *obj = GUI_GetFreeObj();
    struct guiTextSelect *select;
    struct guiBox *box;

    if (obj == NULL)
        return NULL;

    box = &obj->box;
    select = &obj->o.textselect;

    switch (type) {
        case TEXTSELECT_128:
            select->image.file = "images/spin128.bmp";
            break;
        case TEXTSELECT_64:
            select->image.file = "images/spin64.bmp";
            break;
        case TEXTSELECT_96:
            select->image.file = "images/spin96.bmp";
            break;
    }
    select->image.x_off = 0;
    select->image.y_off = 0;

    box->x = x;
    box->y = y;
    if(! LCD_ImageDimensions(select->image.file, &box->width, &box->height)) {
        printf("Couldn't locate file: %s\n", select->image.file);
        box->width = 20;
        box->height = 10;
    }

    obj->Type = TextSelect;
    OBJ_SET_TRANSPARENT(obj, 0); //Even if the bmp has transparency, the redraw function will handle it
    OBJ_SET_USED(obj, 1);
    connect_object(obj);

    select->fontColor = fontColor;
    select->ValueCB   = value_cb;
    select->SelectCB  = select_cb;
    select->cb_data   = cb_data;

    return obj;
}

guiObject_t *GUI_CreateLabel(u16 x, u16 y, const char *(*Callback)(guiObject_t *, void *), struct FontDesc font, void *data)
{
    struct guiObject *obj = GUI_GetFreeObj();
    struct guiLabel  *label;
    struct guiBox    *box;

    if (obj == NULL)
        return NULL;

    box = &obj->box;
    label = &obj->o.label;
    box->x = x;
    box->y = y;
    box->width = 0;
    box->height = 0;

    obj->Type = Label;
    OBJ_SET_TRANSPARENT(obj, 0);  //Deal with transparency during drawing
    OBJ_SET_USED(obj, 1);
    connect_object(obj);

    label->CallBack = Callback;
    label->cb_data = data;
    label->font = font;
    if (! label->font.font)
        label->font.font = DEFAULT_FONT.font;

    return obj;
}

guiObject_t *GUI_CreateImage(u16 x, u16 y, u16 width, u16 height, const char *file)
{
    struct guiObject *obj = GUI_GetFreeObj();
    struct guiImage  *image;
    struct guiBox    *box;

    if (obj == NULL)
        return NULL;

    box = &obj->box;
    image = &obj->o.image;

    image->file = file;
    image->x_off = 0;
    image->y_off = 0;

    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;

    obj->Type = Image;
    OBJ_SET_TRANSPARENT(obj, LCD_ImageIsTransparent(file));
    OBJ_SET_USED(obj, 1);
    connect_object(obj);

    return obj;

}

guiObject_t *GUI_CreateButton(u16 x, u16 y, enum ButtonType type, const char *text,
        u16 fontColor, void (*CallBack)(struct guiObject *obj, void *data), void *cb_data)
{
    struct guiObject *obj    = GUI_GetFreeObj();
    struct guiButton *button;
    struct guiBox    *box;
    u16 text_w, text_h;

    if (obj == NULL)
        return NULL;

    box = &obj->box;
    button = &obj->o.button;

    switch (type) {
        case BUTTON_90: button->image.file = "images/btn90_24.bmp"; break;
        case BUTTON_45: button->image.file = "images/btn46_24.bmp"; break;
        case BUTTON_96x16: button->image.file = "images/btn96_16.bmp"; break;
        case BUTTON_64x16: button->image.file = "images/btn64_16.bmp"; break;
        case BUTTON_48x16: button->image.file = "images/btn48_16.bmp"; break;
        case BUTTON_32x16: button->image.file = "images/btn32_16.bmp"; break;
    }
    button->image.x_off = 0;
    button->image.y_off = 0;

    box->x = x;
    box->y = y;
    if(! LCD_ImageDimensions(button->image.file, &box->width, &box->height)) {
        printf("Couldn't locate file: %s\n", button->image.file);
        box->width = 20;
        box->height = 10;
    }

    obj->Type = Button;
    OBJ_SET_TRANSPARENT(obj, 0); //No need to set transparency since the image cannot be overlapped, and the file can't change
    OBJ_SET_USED(obj, 1);
    connect_object(obj);

    LCD_GetStringDimensions((u8 *) text, &text_w, &text_h);
    button->text = text;
    button->text_x_off = (box->width - text_w) / 2 + x;
    button->text_y_off = (box->height - text_h) / 2 + y + 2;
    button->fontColor = fontColor;
    button->CallBack = CallBack;
    button->cb_data = cb_data;

    return obj;
}

guiObject_t *GUI_CreateListBox(u16 x, u16 y, u16 width, u16 height, u8 item_count, s16 selected,
        const char *(*string_cb)(u8 idx, void *data),
        void (*select_cb)(struct guiObject *obj, u16 selected, void *data),
        void (*longpress_cb)(struct guiObject *obj, u16 selected, void *data),
        void *cb_data)
{
    struct guiObject  *obj = GUI_GetFreeObj();
    struct guiListbox *listbox;
    struct guiBox     *box;
    u16 text_w, text_h;
    s16 pos;

    if (obj == NULL)
        return NULL;

    box = &obj->box;
    listbox = &obj->o.listbox;

    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;

    obj->Type = Listbox;
    OBJ_SET_TRANSPARENT(obj, 0);
    OBJ_SET_USED(obj, 1);
    connect_object(obj);

    LCD_GetCharDimensions('A', &text_w, &text_h);
    listbox->text_height = text_h + 2;  //LINE_SPACING = 2
    listbox->entries_per_page = (height + 2) / listbox->text_height;
    if (listbox->entries_per_page > item_count)
        listbox->entries_per_page = item_count;
    listbox->item_count = item_count;
    listbox->cur_pos = 0;
    if(selected >= 0) {
        pos = selected - (listbox->entries_per_page / 2);
        if (pos < 0)
            pos = 0;
        listbox->cur_pos = pos;
    }
    listbox->selected = selected;
    
    listbox->string_cb = string_cb;
    listbox->select_cb = select_cb;
    listbox->longpress_cb = longpress_cb;
    listbox->cb_data = cb_data;

    return obj;
}

guiObject_t *GUI_CreateXYGraph(u16 x, u16 y, u16 width, u16 height,
                      s16 min_x, s16 min_y, s16 max_x, s16 max_y,
                      u16 gridx, u16 gridy,
                      s16 (*Callback)(s16 xval, void *data),
                      u8 (*point_cb)(s16 *x, s16 *y, u8 pos, void *data),
                      u8 (*touch_cb)(s16 x, s16 y, void *data),
                      void *cb_data)
{
    struct guiObject  *obj   = GUI_GetFreeObj();
    struct guiXYGraph *graph;
    struct guiBox    *box;

    if (obj == NULL)
        return NULL;

    box = &obj->box;
    graph = &obj->o.xy;

    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;

    obj->Type = XYGraph;
    OBJ_SET_TRANSPARENT(obj, 0);
    OBJ_SET_USED(obj, 1);
    connect_object(obj);

    graph->min_x = min_x;
    graph->min_y = min_y;
    graph->max_x = max_x;
    graph->max_y = max_y;
    graph->grid_x = gridx;
    graph->grid_y = gridy;
    graph->CallBack = Callback;
    graph->point_cb = point_cb;
    graph->touch_cb = touch_cb;
    graph->cb_data = cb_data;

    return obj;
}

guiObject_t *GUI_CreateBarGraph(u16 x, u16 y, u16 width, u16 height,
                      s16 min, s16 max, u8 direction,
                      s16 (*Callback)(void *data), void *cb_data)
{
    struct guiObject   *obj   = GUI_GetFreeObj();
    struct guiBarGraph *graph;
    struct guiBox    *box;

    if (obj == NULL)
        return NULL;

    box = &obj->box;
    graph = &obj->o.bar;

    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;

    obj->Type = BarGraph;
    OBJ_SET_TRANSPARENT(obj, 0);
    OBJ_SET_USED(obj, 1);
    connect_object(obj);

    graph->min = min;
    graph->max = max;
    graph->direction = direction;
    graph->CallBack = Callback;
    graph->cb_data = cb_data;

    return obj;
}

u8 coords_in_box(struct guiBox *box, struct touch *coords)
{
    return(coords->x >= box->x && coords->x <= (box->x + box->width)
        && coords->y >= box->y && coords->y <= (box->y + box->height));
}

void GUI_DrawObject(struct guiObject *obj)
{
    struct guiBox *box = &obj->box;
    switch (obj->Type) {
    case UnknownGUI:
        break;
    case Button:
    {
        struct guiButton *button = &obj->o.button;
        LCD_DrawWindowedImageFromFile(box->x, box->y,
                button->image.file, box->width, box->height,
                button->image.x_off, button->image.y_off);
        LCD_SetFontColor(button->fontColor);
        LCD_PrintStringXY(button->text_x_off, button->text_y_off, button->text);
        break;
    }
    case Label:
    {
        GUI_DrawLabel(obj);
        break;
    }
    case Image:
    {
        struct guiImage *image = &obj->o.image;
        LCD_DrawWindowedImageFromFile(box->x, box->y,
                image->file, box->width, box->height,
                image->x_off, image->y_off);
        break;
    }
    case Dialog: {
        struct guiDialog *dialog = &obj->o.dialog;
        //printf("Draw Dialog: X: %d Y: %d WIDTH: %d HEIGHT: %d\n", box->x,
        //        box->y, box->width, box->height);
        LCD_DrawWindowedImageFromFile(box->x, box->y, dialog->image.file,
                box->width, box->height, dialog->image.x_off, dialog->image.y_off);
        LCD_SetFontColor(dialog->titleColor);
        LCD_PrintStringXY(box->x + 5, (box->y + 10), dialog->title);
        LCD_SetFontColor(dialog->fontColor);
        LCD_PrintStringXY(box->x + 5, (box->y + ((box->height / 2) - 4)),
                dialog->text);
        int i;
        for (i=0;i<4;i++) {
            if (dialog->button[i] != NULL) {
                GUI_DrawObject(dialog->button[i]);
            }
        }
        break;
    }
    case CheckBox:
        break;
    case Dropdown:
        break;
    case XYGraph:
        GUI_DrawXYGraph(obj);
        break;
    case BarGraph:
        GUI_DrawBarGraph(obj);
        break;
    case TextSelect:
        GUI_DrawTextSelect(obj);
        break;
    case Listbox:
        GUI_DrawListbox(obj, 1);
        break;
    case Keyboard:
        GUI_DrawKeyboard(obj, NULL, 0);
        break;
    }
    OBJ_SET_DIRTY(obj, 0);
}

void GUI_DrawObjects(void)
{

    struct guiObject *obj = objHEAD;
    while(obj) {
        if(! OBJ_IS_MODAL(obj) || obj->Type == Dialog)
            GUI_DrawObject(obj);
        obj = obj->next;
    }
}

void GUI_RemoveAllObjects()
{
    while(objHEAD)
        GUI_RemoveObj(objHEAD);
}

void GUI_RemoveObj(struct guiObject *obj)
{
    if (obj->Type == Dialog) {
        struct guiDialog *dialog = &obj->o.dialog;
        int i;
        for (i = 0; i < 4; i++)
            if (dialog->button[i])
                GUI_RemoveObj(dialog->button[i]);
    }
    OBJ_SET_USED(obj, 0);
    // Reattach linked list
    struct guiObject *prev = objHEAD;
    if (prev == obj) {
        objHEAD = obj->next;
    } else {
        while(prev) {
            if(prev->next == obj) {
                prev->next = obj->next;
                break;
            }
            prev = prev->next;
        }
    }
    FullRedraw = 1;
}

struct guiObject *GUI_GetFreeObj(void)
{
    int i;
    struct guiObject *obj;
    for (i = 0; i < 256; i++) {
        if (! OBJ_IS_USED(&GUI_Array[i])) {
            obj = &GUI_Array[i];
            obj->next = NULL;
            OBJ_SET_DISABLED(obj, 0);
            OBJ_SET_MODAL(obj, 0);
            OBJ_SET_DIRTY(obj, 1);
            return obj;
        }
    }
    return NULL;
}

void GUI_DrawBackground(u16 x, u16 y, u16 w, u16 h)
{
    if(w == 0 || h == 0)
        return;
    LCD_DrawWindowedImageFromFile(x, y, "images/devo8.bmp", w, h, x, y);
}

void GUI_DrawScreen(void)
{
    /*
     * First we need to draw the main background
     *  */
    GUI_DrawBackground(0, 0, 320, 240);
    /*
     * Then we need to draw any supporting GUI
     */
    GUI_DrawObjects();
    FullRedraw = 0;
}

struct guiObject *GUI_IsModal(void)
{
    struct guiObject *obj = objHEAD;
    while(obj) {
        if(obj->Type == Dialog && OBJ_IS_USED(obj))
            return obj;
        obj = obj->next;
    }
    return NULL;
}

void dgCallback(struct guiObject *obj, void *data)
{
    (void)data;
    struct guiDialogReturn gDR;
    struct guiObject *dlgObj = (struct guiObject *)data;
    struct guiDialog *dialog = &dlgObj->o.dialog;
    if (dialog->button[0] == obj) {
        gDR.buttonPushed = 0;
    }
    if (dialog->button[1] == obj) {
        gDR.buttonPushed = 1;
    }
    if (dialog->button[2] == obj) {
        gDR.buttonPushed = 2;
    }
    if (dialog->button[3] == obj) {
        gDR.buttonPushed = 3;
    }
    gDR.intInput = 0;
    sprintf(gDR.strInput, " ");
    dialog->CallBack(dlgObj, gDR);
}

void GUI_Redraw(struct guiObject *obj)
{
    OBJ_SET_DIRTY(obj, 1);
}

void GUI_RefreshScreen(void)
{
    struct guiObject *obj;
    if (FullRedraw) {
        GUI_DrawScreen();
        return;
    }
    if((obj = GUI_IsModal())) {
        if(OBJ_IS_DIRTY(obj)) {
            GUI_DrawObject(obj);
        }
    } else {
        obj = objHEAD;
        while(obj) {
            if(OBJ_IS_DIRTY(obj)) {
                if(OBJ_IS_TRANSPARENT(obj)) {
                    GUI_DrawBackground(obj->box.x, obj->box.y, obj->box.width, obj->box.height);
                }
                GUI_DrawObject(obj);
            }
            obj = obj->next;
        }
    }
}

void GUI_TouchRelease()
{
    u8 modalActive;
    modalActive = GUI_IsModal() ? 1 : 0;
    struct guiObject *obj = objHEAD;
    while(obj) {
        if (! OBJ_IS_DISABLED(obj)
            && ((modalActive == 0) || OBJ_IS_MODAL(obj)))
        {
            switch (obj->Type) {
            case Keyboard:
            {
                struct guiKeyboard *keyboard = &obj->o.keyboard;
                if(keyboard->last_coords.x || keyboard->last_coords.y) {
                    GUI_DrawKeyboard(obj, NULL, 0);
                    return;
                }
                break;
            }
            default: break;
            }
        }
        obj = obj->next;
    }
}
u8 GUI_CheckTouch(struct touch *coords, u8 long_press)
{
    u8 modalActive;
    modalActive = GUI_IsModal() ? 1 : 0;
    struct guiObject *obj = objHEAD;
    while(obj) {
        if (! OBJ_IS_DISABLED(obj)
            && ((modalActive == 0) || OBJ_IS_MODAL(obj)))
        {
            switch (obj->Type) {
            case UnknownGUI:
                break;
            case Button:
                if (coords_in_box(&obj->box, coords)) {
                    struct guiButton *button = &obj->o.button;
                    if(button->CallBack) {
                        OBJ_SET_DIRTY(obj, 1);
                        button->CallBack(obj, button->cb_data);
                        return 1;
                    }
                    return 0;
                }
                break;
            case TextSelect:
                if(coords_in_box(&obj->box, coords)) {
                    struct guiBox box = obj->box;
                    struct guiTextSelect *select = &obj->o.textselect;
                    box.width = 16;
                    if (coords_in_box(&box, coords)) {
                        if (select->ValueCB) {
                            OBJ_SET_DIRTY(obj, 1);
                            select->ValueCB(obj, long_press ? -2 : -1, select->cb_data);
                            return 1;
                        }
                    }
                    box.x = obj->box.x + obj->box.width - 16;
                    if (coords_in_box(&box, coords)) {
                        if (select->ValueCB) {
                            OBJ_SET_DIRTY(obj, 1);
                            select->ValueCB(obj, long_press ? 2 : 1, select->cb_data);
                            return 1;
                        }
                    }
                    if (select->SelectCB) {
                        OBJ_SET_DIRTY(obj, 1);
                        select->SelectCB(obj, select->cb_data);
                        return 1;
                    }
                    return 0;
                }
                break;
            case Listbox:
                if(coords_in_box(&obj->box, coords)) {
                    return GUI_TouchListbox(obj, coords, long_press);
                }
                break;
            case Dialog:
                break; /* Dialogs are handled by buttons */
            case Label:
                break;
            case Image:
                break;
            case CheckBox:
                break;
            case Dropdown:
                break;
            case XYGraph:
                if(coords_in_box(&obj->box, coords)) {
                    struct guiXYGraph *graph = &obj->o.xy;
                    if (graph->touch_cb) {
                        s32 x, y;
                        x = (s32)(coords->x - obj->box.x) * (1 + graph->max_x - graph->min_x) / obj->box.width + graph->min_x;
                        y = (s32)(obj->box.height -1 - (coords->y - obj->box.y))
                            * (1 + graph->max_y - graph->min_y) / obj->box.height + graph->min_y;
                        if(graph->touch_cb(x, y, graph->cb_data)) {
                            OBJ_SET_DIRTY(obj, 1);
                            return 1;
                        }
                    }
                    return 0;
                }
                break;
            case BarGraph:
                break;
            case Keyboard:
                //Note that this only works because the keyboard encompasses the whole screen
                return GUI_DrawKeyboard(obj, coords, long_press);
                break;
            }
        }
        obj = obj->next;
    }
    return 0;
}

void GUI_DrawLabel(struct guiObject *obj)
{
    struct guiLabel *label = &obj->o.label;
    const char *str;
    u16 old_w = obj->box.width;
    u16 old_h = obj->box.height;
    if (label->CallBack)
        str = label->CallBack(obj, label->cb_data);
    else
        str = (const char *)label->cb_data;
    u8 old_font = LCD_SetFont(label->font.font);
    LCD_GetStringDimensions((const u8 *)str, &obj->box.width, &obj->box.height);
    if (old_w < obj->box.width)
        old_w = obj->box.width;
    if (old_h < obj->box.height)
        old_h = obj->box.height;
    GUI_DrawBackground(obj->box.x, obj->box.y, old_w, old_h);
    LCD_SetFontColor(label->font.color);
    LCD_PrintStringXY(obj->box.x, obj->box.y, str);
    LCD_SetFont(old_font);
}

void GUI_DrawXYGraph(struct guiObject *obj)
{
    struct guiBox *box = &obj->box;
    struct guiXYGraph *graph = &obj->o.xy;
    u32 x, y;

    inline u32 VAL_TO_X(s32 xval)
    {
        return box->x + (xval - graph->min_x) * box->width / (1 + graph->max_x - graph->min_x);
    }
    inline u32 VAL_TO_Y(s32 yval) {
        return box->y + box->height - (yval - graph->min_y) * box->height / (1 + graph->max_y - graph->min_y);
    }
    LCD_FillRect(box->x, box->y, box->width, box->height, 0x0000);
    if (graph->grid_x) {
        int xval;
        for (xval = graph->min_x + graph->grid_x; xval < graph->max_x; xval += graph->grid_x) {
            if (! xval)
                continue;
            x = VAL_TO_X(xval);
            //LCD_DrawDashedVLine(x, box->y, box->height, 5, RGB888_to_RGB565(0x30, 0x30, 0x30));
            LCD_DrawFastVLine(x, box->y, box->height, RGB888_to_RGB565(0x30, 0x30, 0x30));
        }
    }
    if (graph->grid_y) {
        int yval;
        for (yval = graph->min_y + graph->grid_y; yval < graph->max_y; yval += graph->grid_y) {
            if (! yval)
                continue;
            y = VAL_TO_Y(yval);
            //LCD_DrawDashedHLine(box->x, y, box->width, 5, RGB888_to_RGB565(0x30, 0x30, 0x30));
            LCD_DrawFastHLine(box->x, y, box->width, RGB888_to_RGB565(0x30, 0x30, 0x30));
        }
    }
    if (graph->min_x < 0 && graph->max_x > 0) {
        int x = box->x + box->width * (0 - graph->min_x) / (graph->max_x - graph->min_x);
        LCD_DrawFastVLine(x, box->y, box->height, 0xFFFF);
    }
    if (graph->min_y < 0 && graph->max_y > 0) {
        y = box->y + box->height - box->height * (0 - graph->min_y) / (graph->max_y - graph->min_y);
        LCD_DrawFastHLine(box->x, y, box->width, 0xFFFF);
    }
    u16 lastx, lasty;
    for (x = 0; x < box->width; x++) {
        s32 xval, yval;
        xval = graph->min_x + x * (1 + graph->max_x - graph->min_x) / box->width;
        yval = graph->CallBack(xval, graph->cb_data);
        y = (yval - graph->min_y) * box->height / (1 + graph->max_y - graph->min_y);
        //printf("(%d, %d) -> (%d, %d)\n", (int)x, (int)y, (int)xval, (int)yval);
        if (x != 0) {
            LCD_DrawLine(lastx, lasty, x + box->x, box->y + box->height - y - 1, 0xFFE0); //Yellow
        }
        lastx = x + box->x;
        lasty = box->y + box->height - y - 1;
    }
    LCD_DrawStop();
    if (graph->point_cb) {
        u8 pos = 0;
        s16 xval, yval;
        while (graph->point_cb(&xval, &yval, pos++, graph->cb_data)) {
            s16 x1 = VAL_TO_X(xval);
            s16 y1 = VAL_TO_Y(yval);
            s16 x2 = x1 + 2;
            s16 y2 = y1 + 2;
            //bounds check
            x1 = ( x1 < 2 + box->x) ? box->x : x1 - 2;
            y1 = ( y1 < 2 + box->y) ? box->y : y1 - 2;
            if ( x2 >= box->x + box->width)
                x2 = box->x + box->width - 1;
            if ( y2 >= box->y + box->height)
                y2 = box->y + box->height - 1;
            LCD_FillRect(x1, y1, x2 - x1 + 1, y2 - y1 + 1, RGB888_to_RGB565(0x00, 0xFF, 0xFF));
        }
    }
}

void GUI_DrawBarGraph(struct guiObject *obj)
{
#define TRANSPARENT_BARGRAPH
#define TRIM_THICKNESS 10
#define TRIM_MARGIN 1
    struct guiBox *box = &obj->box;
    struct guiBarGraph *graph = &obj->o.bar;
    int height = box->height - 2;
    int width  = box->width - 2;
    int x = box->x + 1;
    int y = box->y + 1;

    LCD_DrawRect(box->x, box->y, box->width, box->height, 0xFFFF);
    
    s32 val = graph->CallBack(graph->cb_data);

    switch(graph->direction) {
    case BAR_HORIZONTAL: {
        val = width * (val - graph->min) / (graph->max - graph->min);
        LCD_FillRect(x, y, val, height, 0xFFE0);
#ifdef TRANSPARENT_BARGRAPH
        GUI_DrawBackground(x + val, y, width - val, height);
#else
        LCD_FillRect(x + val, y, width - val, height, 0x0000);
#endif
        break;
    }
    case BAR_VERTICAL: {
        val = height * (val - graph->min) / (graph->max - graph->min);
        LCD_FillRect(x, y + (height - val), width, val, 0xFFE0);
#ifdef TRANSPARENT_BARGRAPH
        GUI_DrawBackground(x, y, width, height - val);
#else
        LCD_FillRect(x, y, width, height - val, 0x0000);
#endif
        break;
    }
    case TRIM_HORIZONTAL: {
        val = (TRIM_THICKNESS / 2) + (width - TRIM_THICKNESS) * (val - graph->min) / (graph->max - graph->min);
        GUI_DrawBackground(x, y, width, height);
//        LCD_DrawFastHLine(x, y + height / 2, width, 0x0000); //Main axis
        LCD_DrawFastVLine(x + width / 2, y, height, 0xFFFF); //Center
        LCD_FillRect(x + val - TRIM_THICKNESS / 2, y + TRIM_MARGIN, TRIM_THICKNESS, height - TRIM_MARGIN * 2, 0x0000);
        break;
    }
    case TRIM_VERTICAL: {
        val = (TRIM_THICKNESS / 2) + (height - TRIM_THICKNESS) * (val - graph->min) / (graph->max - graph->min);
        GUI_DrawBackground(x, y, width, height);
//        LCD_DrawFastVLine(x + width / 2, y, height, 0xFFFF); //Main axis
        LCD_DrawFastHLine(x, y + height / 2, width, 0xFFFF); //Center
        LCD_FillRect(x + TRIM_MARGIN, y + (height - val) - TRIM_THICKNESS / 2, width - TRIM_MARGIN * 2, TRIM_THICKNESS, 0x0000);
        break;
    }
    }
}

void GUI_DrawTextSelect(struct guiObject *obj)
{
    u16 x, y, w, h;
    struct guiBox *box = &obj->box;
    struct guiTextSelect *select = &obj->o.textselect;
    LCD_DrawWindowedImageFromFile(box->x, box->y, select->image.file, box->width, box->height, select->image.x_off, select->image.y_off);
    const char *str =select->ValueCB(obj, 0, select->cb_data);
    LCD_SetFontColor(select->fontColor);
    LCD_GetStringDimensions((const u8 *)str, &w, &h);
    x = box->x + (box->width - w) / 2;
    y = box->y + 2 + (box->height - h) / 2;
    LCD_PrintStringXY(x, y, str);
}

s32 GUI_TextSelectHelper(s32 value, s32 min, s32 max, s8 dir, u8 shortstep, u8 longstep, u8 *_changed)
{
    u8 changed = 0;
    if (dir > 0) {
        if (value < max) {
           value += (dir > 1) ? longstep : shortstep;
           if (value > max)
               value = max;
           changed = 1;
        }
    } else if (dir < 0) {
        if (value > min) {
           value -= (dir < -1) ? longstep : shortstep;
           if (value < min)
               value = min;
           changed = 1;
        }
    }
    if(_changed)
        *_changed = changed;
    return value;
}

void GUI_DrawListbox(struct guiObject *obj, u8 redraw_all)
{
    #define BAR_HEIGHT 10
    #define FILL        Display.listbox.bg_color    // RGB888_to_RGB565(0xaa, 0xaa, 0xaa)
    #define TEXT        Display.listbox.fg_color    // 0x0000
    #define SELECT      Display.listbox.bg_select   // RGB888_to_RGB565(0x44, 0x44, 0x44)
    #define SELECT_TXT  Display.listbox.fg_select   // 0xFFFF
    #define BAR_BG      Display.listbox.bg_bar      // RGB888_to_RGB565(0x44, 0x44, 0x44)
    #define BAR_FG      Display.listbox.fg_bar      // RGB888_to_RGB565(0xaa, 0xaa, 0xaa)
    
    u8 i, old;
    u8 bar;
    struct guiListbox *listbox = &obj->o.listbox;
    if (redraw_all) {
        LCD_FillRect(obj->box.x, obj->box.y, obj->box.width - ARROW_WIDTH, obj->box.height, FILL);
        LCD_DrawWindowedImageFromFile(obj->box.x + obj->box.width - ARROW_WIDTH, obj->box.y,
                ARROW_FILE, ARROW_WIDTH, ARROW_HEIGHT, ARROW_UP, 0);
        LCD_DrawWindowedImageFromFile(obj->box.x + obj->box.width - ARROW_WIDTH, obj->box.y + obj->box.height - ARROW_HEIGHT,
                ARROW_FILE, ARROW_WIDTH, ARROW_HEIGHT, ARROW_DOWN, 0);
    }
    u16 denom = (listbox->item_count == listbox->entries_per_page) ?  1 : listbox->item_count - listbox->entries_per_page;
    bar = listbox->cur_pos * (obj->box.height - 2 * ARROW_HEIGHT - BAR_HEIGHT) / denom;
    LCD_FillRect(obj->box.x + obj->box.width - ARROW_WIDTH, obj->box.y + ARROW_HEIGHT, ARROW_WIDTH, obj->box.height - 2 * ARROW_HEIGHT, BAR_BG);
    LCD_FillRect(obj->box.x + obj->box.width - ARROW_WIDTH, obj->box.y + ARROW_HEIGHT + bar, ARROW_WIDTH, BAR_HEIGHT, BAR_FG);
    LCD_SetXY(obj->box.x + 5, obj->box.y + 1);
    if(listbox->selected >= listbox->cur_pos && listbox->selected < listbox->cur_pos + listbox->entries_per_page)
        LCD_FillRect(obj->box.x, obj->box.y + (listbox->selected - listbox->cur_pos) * listbox->text_height,
                     obj->box.width - ARROW_WIDTH, listbox->text_height, SELECT);
    old = LCD_SetFont(Display.listbox.font ? Display.listbox.font : DEFAULT_FONT.font);
    for(i = 0; i < listbox->entries_per_page; i++) {
        const char *str = listbox->string_cb(i + listbox->cur_pos, listbox->cb_data);
        LCD_SetFontColor(i + listbox->cur_pos == listbox->selected ? SELECT_TXT : TEXT);
       
        LCD_PrintString(str);
        LCD_PrintString("\n");
    }
    LCD_SetFont(old);
}

u8 GUI_TouchListbox(struct guiObject *obj, struct touch *coords, u8 long_press)
{
    struct guiListbox *listbox = &obj->o.listbox;
    struct guiBox box;
    u8 i;
    box.x = obj->box.x + obj->box.width - ARROW_WIDTH;
    box.y = obj->box.y;
    box.width = ARROW_WIDTH;
    box.height = ARROW_HEIGHT;
    if(coords_in_box(&box, coords)) {
        s16 new_pos = (s16)listbox->cur_pos - (long_press ? listbox->entries_per_page : 1);
        if (new_pos < 0)
            new_pos = 0;
        if(listbox->cur_pos != new_pos) {
            listbox->cur_pos = (u16)new_pos;
            OBJ_SET_DIRTY(obj, 1);
            return 1;
        }
        return 0;
    }
    box.y = obj->box.y + obj->box.height - ARROW_HEIGHT;
    if(coords_in_box(&box, coords)) {
        s16 new_pos = (s16)listbox->cur_pos + (long_press ? listbox->entries_per_page : 1);
        if (new_pos > listbox->item_count - listbox->entries_per_page)
            new_pos = listbox->item_count - listbox->entries_per_page;
        if(listbox->cur_pos != new_pos) {
            listbox->cur_pos = (u16)new_pos;
            OBJ_SET_DIRTY(obj, 1);
            return 1;
        }
        return 0;
    }
    box.x = obj->box.x;
    box.width = obj->box.width - ARROW_WIDTH;
    box.height = listbox->text_height;
    for (i = 0; i < listbox->entries_per_page; i++) {
        box.y = obj->box.y + i * listbox->text_height;
        if (coords_in_box(&box, coords)) {
            u8 selected = i + listbox->cur_pos;
            if (selected != listbox->selected) {
                listbox->selected = selected;
                if (listbox->select_cb)
                    listbox->select_cb(obj, (u16)selected, listbox->cb_data);
                OBJ_SET_DIRTY(obj, 1);
                return 1;
            } else if (long_press && listbox->longpress_cb) {
                listbox->longpress_cb(obj, (u16)selected, listbox->cb_data);
            }
            return 0;
        }
    }
    return 0;
}
