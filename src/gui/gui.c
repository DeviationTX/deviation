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
#define ENABLE_GUIOBJECT
#include "gui.h"

struct guiObject GUI_Array[128];
struct guiObject *objHEAD = NULL;
struct guiButton GUI_Button_Array[64];
struct guiLabel GUI_Label_Array[64];
struct guiFrame GUI_Frame_Array[16];
struct guiDialog GUI_Dialog_Array[8];
struct guiXYGraph GUI_XYGraph_Array[2];
struct guiBarGraph GUI_BarGraph_Array[32];
struct guiTextSelect GUI_TextSelect_Array[16];
static u8 FullRedraw;

static void GUI_DrawObject(struct guiObject *obj);
static struct guiObject *GUI_GetFreeObj(void);
static void *GUI_GetFreeGUIObj(enum GUIType guiType);
static void dgCallback(struct guiObject *obj);
static void GUI_DrawBarGraph(struct guiObject *obj);
static void GUI_DrawXYGraph(struct guiObject *obj);
static void GUI_DrawTextSelect(struct guiObject *obj);

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
    struct guiDialog *dialog = GUI_GetFreeGUIObj(Dialog);
    struct guiObject *obj = GUI_GetFreeObj();
    struct guiBox *box ;

    if (dialog == NULL || obj == NULL)
        return NULL;

    box = &obj->box;

    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;
    box->image.file = "dialog.bmp";
    box->image.x_off = 0;
    box->image.y_off = 0;

    obj->Type = Dialog;
    obj->CallBack = (void *) 0x1;
    obj->widget = dialog;
    OBJ_SET_TRANSPARENT(obj, LCD_ImageIsTransparent(box->image.file));
    OBJ_SET_MODAL(obj, 1);
    connect_object(obj);

    dialog->text = text;
    dialog->title = title;
    dialog->fontColor = fontColor;
    dialog->titleColor = titleColor;
    dialog->Type = dgType;
    dialog->CallBack = *CallBack;
    dialog->inuse = 1;

    dialog->button[0] = NULL;
    dialog->button[1] = NULL;
    dialog->button[2] = NULL;
    dialog->button[3] = NULL;

    switch (dgType) {
    case dtOk:
        dialog->button[0] = GUI_CreateButton(((x + width) / 2) - 10,
                ((y + height) - 27), 89, 23, "Ok", 0x0000, dgCallback);
        OBJ_SET_MODAL(dialog->button[0], 1);
        dialog->button[0]->parent = obj;
        break;
    case dtOkCancel:
        break;
    }

    return obj;

}

guiObject_t *GUI_CreateTextSelect(u16 x, u16 y, u16 width, u16 height, u16 fontColor,
        void (*select_cb)(guiObject_t *obj, void *data),
        const char *(*value_cb)(guiObject_t *obj, int value, void *data),
        void *cb_data)
{
    struct guiTextSelect *select = GUI_GetFreeGUIObj(TextSelect);
    struct guiObject *obj = GUI_GetFreeObj();
    struct guiBox *box;

    if (select == NULL || obj == NULL)
        return NULL;

    box = &obj->box;

    box->image.file = "textselect.bmp";
    box->image.x_off = 0;
    box->image.y_off = 0;

    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;

    obj->Type = TextSelect;
    obj->CallBack = (void *) 0x1;
    obj->widget = select;
    OBJ_SET_TRANSPARENT(obj, 0); //Even if the bmp has transparency, the redraw function will handle it
    connect_object(obj);

    select->fontColor = fontColor;
    select->ValueCB   = value_cb;
    select->SelectCB  = select_cb;
    select->cb_data   = cb_data;
    select->inuse = 1;

    return obj;
}

guiObject_t *GUI_CreateLabel(u16 x, u16 y, const char *text, u16 fontColor)
{
    struct guiLabel  *label = GUI_GetFreeGUIObj(Label);
    struct guiObject *obj = GUI_GetFreeObj();
    struct guiBox    *box;

    if (label == NULL || obj == NULL)
        return NULL;

    box = &obj->box;
    box->x = x;
    box->y = y;
    LCD_GetStringDimensions((const u8 *)text, &box->width, &box->height);

    obj->Type = Label;
    obj->CallBack = (void *) 0x1; /* no call back yet for labels */
    obj->widget = label;
    OBJ_SET_TRANSPARENT(obj, 1);
    connect_object(obj);

    label->text = text;
    label->fontColor = fontColor;
    label->inuse = 1;

    return obj;
}

guiObject_t *GUI_CreateFrame(u16 x, u16 y, u16 width, u16 height, const char *image)
{
    struct guiFrame  *frame = GUI_GetFreeGUIObj(Frame);
    struct guiObject *obj = GUI_GetFreeObj();
    struct guiBox    *box;

    if (frame == NULL || obj == NULL)
        return NULL;

    box = &obj->box;

    box->image.file = image;
    box->image.x_off = 0;
    box->image.y_off = 0;

    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;

    obj->Type = Frame;
    obj->CallBack = (void *) 0x1;
    obj->widget = frame;
    OBJ_SET_TRANSPARENT(obj, LCD_ImageIsTransparent(image));
    connect_object(obj);

    frame->inuse = 1;

    return obj;

}

guiObject_t *GUI_CreateButton(u16 x, u16 y, u16 width, u16 height, const char *text,
        u16 fontColor, void (*CallBack)(struct guiObject *obj))
{
    struct guiButton *button = GUI_GetFreeGUIObj(Button);
    struct guiObject *obj    = GUI_GetFreeObj();
    struct guiBox *box;
    u16 text_w, text_h;

    if (button == NULL || obj == NULL)
        return NULL;

    box = &obj->box;

    box->image.file = "gui.bmp";
    box->image.x_off = 0;
    box->image.y_off = 0;

    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;

    obj->Type = Button;
    obj->widget = button;
    obj->CallBack = *CallBack;
    OBJ_SET_TRANSPARENT(obj, LCD_ImageIsTransparent(box->image.file));
    connect_object(obj);

    LCD_GetStringDimensions((u8 *) text, &text_w, &text_h);
    button->inuse = 1;
    button->text = text;
    button->text_x_off = (width - text_w) / 2 + x;
    button->text_y_off = (height - text_h) / 2 + y;
    button->fontColor = fontColor;

    return obj;
}

guiObject_t *GUI_CreateXYGraph(u16 x, u16 y, u16 width, u16 height,
                      s16 min_x, s16 min_y, s16 max_x, s16 max_y,
                      s16 (*Callback)(s16 xval, void *data), void *cb_data)
{
    struct guiXYGraph *graph = GUI_GetFreeGUIObj(XYGraph);
    struct guiObject  *obj   = GUI_GetFreeObj();
    struct guiBox    *box;

    if (graph == NULL || obj == NULL)
        return NULL;

    box = &obj->box;

    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;

    obj->Type = XYGraph;
    obj->CallBack = (void *) 0x1;
    obj->widget = graph;
    OBJ_SET_TRANSPARENT(obj, 0);
    connect_object(obj);

    graph->min_x = min_x;
    graph->min_y = min_y;
    graph->max_x = max_x;
    graph->max_y = max_y;
    graph->inuse = 1;
    graph->CallBack = Callback;
    graph->cb_data = cb_data;

    return obj;
}

guiObject_t *GUI_CreateBarGraph(u16 x, u16 y, u16 width, u16 height,
                      s16 min, s16 max, u8 direction,
                      s16 (*Callback)(void *data), void *cb_data)
{
    struct guiBarGraph *graph = GUI_GetFreeGUIObj(BarGraph);
    struct guiObject   *obj   = GUI_GetFreeObj();
    struct guiBox    *box;

    if (graph == NULL || obj == NULL)
        return NULL;

    box = &obj->box;

    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;

    obj->Type = BarGraph;
    obj->CallBack = (void *) 0x1;
    obj->widget = graph;
    OBJ_SET_TRANSPARENT(obj, 0);
    connect_object(obj);

    graph->min = min;
    graph->max = max;
    graph->direction = direction;
    graph->inuse = 1;
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
    if (obj->CallBack != 0) {
        switch (obj->Type) {
        case UnknownGUI:
            break;
        case Button:
        {
            struct guiButton *button = (struct guiButton *)obj->widget;
            LCD_DrawWindowedImageFromFile(box->x, box->y,
                    box->image.file, box->width, box->height,
                    box->image.x_off, box->image.y_off);
            LCD_SetFontColor(button->fontColor);
            LCD_PrintStringXY(button->text_x_off, button->text_y_off, button->text);
            break;
        }
        case Label:
        {
            struct guiLabel *label = (struct guiLabel *)obj->widget;
            LCD_SetFontColor(label->fontColor);
            LCD_PrintStringXY(box->x, box->y, label->text);
            break;
        }
        case Frame:
        {
            LCD_DrawWindowedImageFromFile(box->x, box->y,
                    box->image.file, box->width, box->height,
                    box->image.x_off, box->image.y_off);
            break;
        }
        case Dialog: {
            struct guiDialog *dialog = (struct guiDialog *)obj->widget;
            printf("Draw Dialog: X: %d Y: %d WIDTH: %d HEIGHT: %d\n", box->x,
                    box->y, box->width, box->height);
            LCD_DrawWindowedImageFromFile(box->x, box->y, box->image.file,
                    box->width, box->height, box->image.x_off, box->image.y_off);
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
        }
    }
    OBJ_SET_DIRTY(obj, 0);
}

void GUI_DrawObjects(void)
{

    struct guiObject *obj = objHEAD;
    while(obj) {
        if(! obj->parent)
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
    switch (obj->Type) {
    case UnknownGUI:
        break;
    case Button: 
        ((struct guiButton *)obj->widget)->inuse = 0;
        break;
    case Label:
        ((struct guiLabel *)obj->widget)->inuse = 0;
        break;
    case Frame:
        ((struct guiFrame *)obj->widget)->inuse = 0;
        break;
    case Dialog:
    {
        struct guiDialog *dialog = (struct guiDialog *)obj->widget;
        dialog->inuse = 0;
        int i;
        for (i = 0; i < 4; i++)
            if (dialog->button[i])
                GUI_RemoveObj(dialog->button[i]);
        break;
    }
    case CheckBox:
        break;
    case Dropdown:
        break;
    case XYGraph:
        ((struct guiXYGraph *)obj->widget)->inuse = 0;
        break;
    case BarGraph:
        ((struct guiBarGraph *)obj->widget)->inuse = 0;
        break;
    case TextSelect:
        ((struct guiTextSelect *)obj->widget)->inuse = 0;
        break;
    }
    obj->CallBack = (void *)0;
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
        if (GUI_Array[i].CallBack == 0) {
            obj = &GUI_Array[i];
            obj->next = NULL;
            obj->parent = NULL;
            OBJ_SET_DISABLED(obj, 0);
            OBJ_SET_MODAL(obj, 0);
            OBJ_SET_DIRTY(obj, 1);
            return obj;
        }
    }
    return NULL;
}

void *GUI_GetFreeGUIObj(enum GUIType guiType)
{
    int i;
    for (i = 0; i < 256; i++) {
        switch (guiType) {
        case UnknownGUI:
            break;
        case Button:
            if (GUI_Button_Array[i].inuse == 0) {
                return &GUI_Button_Array[i];
            }
            break;
        case Label:
            if (GUI_Label_Array[i].inuse == 0) {
                return &GUI_Label_Array[i];
            }
            break;
        case Frame:
            if (GUI_Frame_Array[i].inuse == 0) {
                return &GUI_Frame_Array[i];
            }
            break;
        case Dialog:
            if (GUI_Dialog_Array[i].inuse == 0) {
                return &GUI_Dialog_Array[i];
            }
            break;
        case CheckBox:
            break;
        case Dropdown:
            break;
        case XYGraph:
            if (GUI_XYGraph_Array[i].inuse == 0) {
                return &GUI_XYGraph_Array[i];
            }
            break;
        case BarGraph:
            if (GUI_BarGraph_Array[i].inuse == 0) {
                return &GUI_BarGraph_Array[i];
            }
            break;
        case TextSelect:
            if (GUI_TextSelect_Array[i].inuse == 0) {
                return &GUI_TextSelect_Array[i];
            }
            break;
        }
    }
    return NULL;
}

void GUI_DrawBackground(u16 x, u16 y, u16 w, u16 h)
{
    if(w == 0 || h == 0)
        return;
    LCD_DrawWindowedImageFromFile(x, y, "devo8.bmp", w, h, x, y);
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

u8 GUI_IsModal(void)
{
    return GUI_Dialog_Array[0].inuse;
}

void dgCallback(struct guiObject *obj)
{
    struct guiDialogReturn gDR;
    struct guiDialog *dialog = (struct guiDialog *)obj->parent->widget;
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
    dialog->CallBack(obj->parent, gDR);
}

void GUI_Redraw(struct guiObject *obj)
{
    OBJ_SET_DIRTY(obj, 1);
}

void GUI_RefreshScreen(void)
{
    if (FullRedraw) {
        GUI_DrawScreen();
        return;
    }
    if(GUI_IsModal()) {
        struct guiObject *obj = objHEAD;
        while(obj) {
            if(OBJ_IS_MODAL(obj) && OBJ_IS_DIRTY(obj)) {
                if (obj->parent)
                    GUI_DrawObject(obj->parent);
                else
                    GUI_DrawObject(obj);
            }
            obj = obj->next;
        }
    } else {
        struct guiObject *obj = objHEAD;
        while(obj) {
            if(OBJ_IS_DIRTY(obj)) {
                if(OBJ_IS_TRANSPARENT(obj)) {
                    // Labels are special because they can change size
                    if (obj->Type == Label) {
                        struct guiLabel *label = (struct guiLabel *)obj->widget;
                        LCD_GetStringDimensions((const u8 *)label->text, &obj->box.width, &obj->box.height);
                    }
                    GUI_DrawBackground(obj->box.x, obj->box.y, obj->box.width, obj->box.height);
                }
                GUI_DrawObject(obj);
            }
            obj = obj->next;
        }
    }
}

u8 GUI_CheckTouch(struct touch coords)
{
    u8 redraw = 0;
    u8 modalActive;
    modalActive = GUI_IsModal();
    struct guiObject *obj = objHEAD;
    while(obj) {
        if ((obj->CallBack != 0) && ! OBJ_IS_DISABLED(obj)
                && ((modalActive == 0) || OBJ_IS_MODAL(obj)))
        {
            switch (obj->Type) {
            case UnknownGUI:
                break;
            case Button:
                if (coords_in_box(&obj->box, &coords)) {
                    obj->CallBack(obj);
                    OBJ_SET_DIRTY(obj, 1);
                    redraw = 1;
                }
                break;
            case TextSelect:
                if(coords_in_box(&obj->box, &coords)) {
                    struct guiBox box = obj->box;
                    struct guiTextSelect *select = (struct guiTextSelect *)obj->widget;
                    redraw = 1;
                    OBJ_SET_DIRTY(obj, 1);
                    box.width = 16;
                    if (coords_in_box(&box, &coords)) {
                        if (select->ValueCB)
                            select->ValueCB(obj, -1, select->cb_data);
                        break;
                    }
                    box.x = obj->box.x + obj->box.width - 16;
                    if (coords_in_box(&box, &coords)) {
                        if (select->ValueCB)
                            select->ValueCB(obj, 1, select->cb_data);
                        break;
                    }
                    if (select->SelectCB)
                        select->SelectCB(obj, select->cb_data);
                }
                break;
            case Dialog:
                break; /* Dialogs are handled by buttons */
            case Label:
                break;
            case Frame:
                break;
            case CheckBox:
                break;
            case Dropdown:
                break;
            case XYGraph:
                break;
            case BarGraph:
                break;
            }
        }
        obj = obj->next;
    }
    return redraw;
}

void GUI_DrawXYGraph(struct guiObject *obj)
{
    struct guiBox *box = &obj->box;
    struct guiXYGraph *graph = (struct guiXYGraph *)obj->widget;
    u32 x, y;

    LCD_FillRect(box->x, box->y, box->width, box->height, 0x0000);
    if (graph->min_x < 0 && graph->max_x > 0) {
        int x = box->x + box->width * (0 - graph->min_x) / (graph->max_x - graph->min_x);
        LCD_DrawFastVLine(x, box->y, box->height, 0xFFFF);
    }
    if (graph->min_y < 0 && graph->max_y > 0) {
        y = box->y + box->height - box->height * (0 - graph->min_y) / (graph->max_y - graph->min_y);
        LCD_DrawFastHLine(box->x, y, box->width, 0xFFFF);
    }
    for (x = 0; x < box->width; x++) {
        s32 xval, yval;
        xval = graph->min_x + x * (graph->max_x - graph->min_x) / box->width;
        yval = graph->CallBack(xval, graph->cb_data);
        y = (yval - graph->min_y) * box->height / (graph->max_y - graph->min_y);
        //printf("(%d, %d) -> (%d, %d)\n", (int)x, (int)y, (int)xval, (int)yval);
        LCD_DrawPixelXY(x + box->x, box->y + box->height - y , 0xFFE0); //Yellow
    }
}

void GUI_DrawBarGraph(struct guiObject *obj)
{
#define TRANSPARENT_BARGRAPH
    struct guiBox *box = &obj->box;
    struct guiBarGraph *graph = (struct guiBarGraph *)obj->widget;
    int height = box->height - 2;
    int width  = box->width - 2;
    int x = box->x + 1;
    int y = box->y + 1;

    LCD_DrawRect(box->x, box->y, box->width, box->height, 0xFFFF);
    
    s32 val = graph->CallBack(graph->cb_data);

    if (graph->direction == BAR_HORIZONTAL) {
        val = width * (val - graph->min) / (graph->max - graph->min);
        LCD_FillRect(x, y, val, height, 0xFFE0);
#ifdef TRANSPARENT_BARGRAPH
        GUI_DrawBackground(x + val, y, width - val, height);
#else
        LCD_FillRect(x + val, y, width - val, height, 0x0000);
#endif
    } else {
        val = height * (val - graph->min) / (graph->max - graph->min);
        LCD_FillRect(x, y + (height - val), width, val, 0xFFE0);
#ifdef TRANSPARENT_BARGRAPH
        GUI_DrawBackground(x, y, width, height - val);
#else
        LCD_FillRect(x, y, width, height - val, 0x0000);
#endif
    }
}

void GUI_DrawTextSelect(struct guiObject *obj)
{
    u16 x, y, w, h;
    struct guiBox *box = &obj->box;
    struct guiTextSelect *select = (struct guiTextSelect *)obj->widget;
    LCD_DrawWindowedImageFromFile(box->x, box->y, box->image.file, box->width, box->height, box->image.x_off, box->image.y_off);
    const char *str =select->ValueCB(obj, 0, select->cb_data);
    LCD_SetFontColor(select->fontColor);
    LCD_GetStringDimensions((const u8 *)str, &w, &h);
    x = box->x + (box->width - w) / 2;
    y = box->y + (box->height - h) / 2;
    LCD_PrintStringXY(x, y, str);
}
