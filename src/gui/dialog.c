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

#include "common.h"
#define ENABLE_GUIOBJECT
#include "gui.h"
#include "config/display.h"

static void dlgbut_pressok_cb(struct guiObject *obj, void *data);
static void dlgbut_presscancel_cb(struct guiObject *obj, void *data);
const char *dlgbut_strok_cb(struct guiObject *obj, void *data);
const char *dlgbut_strcancel_cb(struct guiObject *obj, void *data);

guiObject_t *GUI_CreateDialog(u16 x, u16 y, u16 width, u16 height, const char *title,
        const char *(*string_cb)(guiObject_t *obj, void *data),
        void (*CallBack)(u8 state, void *data),
        enum DialogType dgType, void *data)
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

    obj->Type = Dialog;
    OBJ_SET_TRANSPARENT(obj, 0);
    OBJ_SET_MODAL(obj, 1);
    connect_object(obj);

    dialog->string_cb = string_cb;
    dialog->title = title;
    dialog->CallBack = *CallBack;
    dialog->cbData = data;
    dialog->txtbox.x = x + 10;
    dialog->txtbox.y = 0;
    dialog->txtbox.width = 0;
    dialog->txtbox.height = 0;

    struct guiObject *but;
    switch (dgType) {
    case dtOk:
        but = GUI_CreateButton(x + (width - 96) / 2, y + height - 27,
                        BUTTON_96, dlgbut_strok_cb, 0x0000, dlgbut_pressok_cb, obj);
        OBJ_SET_MODAL(but, 1);
        break;
    case dtOkCancel:
        but = GUI_CreateButton(x + width - 5 - 48, y + height - 27,
                         BUTTON_48, dlgbut_strok_cb, 0x0000, dlgbut_pressok_cb, obj);
        OBJ_SET_MODAL(but, 1);
        but = GUI_CreateButton(x + width -10 - 48 - 96,
                         y + height - 27,
                         BUTTON_96, dlgbut_strcancel_cb, 0x0000, dlgbut_presscancel_cb, obj);
        OBJ_SET_MODAL(but, 1);
        break;
    }
    GUI_HandleModalButtons(1);
    objSELECTED = but;
    return obj;
}

void GUI_DrawDialog(struct guiObject *obj)
{
    struct guiBox *box = &obj->box;
    struct guiDialog *dialog = &obj->o.dialog;
    if (dialog->txtbox.height == 0) {
        LCD_DrawRect(box->x, box->y, box->width, box->height, DIALOGTITLE_FONT.fill_color);
        LCD_FillRect(box->x, box->y, box->width, 24, DIALOGTITLE_FONT.fill_color);
        LCD_FillRect(box->x + 1, box->y + 24, box->width - 2, box->height - 25, DIALOGBODY_FONT.fill_color);
        LCD_SetFontColor(DIALOGTITLE_FONT.font_color);
        LCD_PrintStringXY(dialog->txtbox.x, (box->y + 5), dialog->title);
    } else if(dialog->txtbox.width) {
        // NOTE: We assume all redraw events after the 1st are incremental!
        GUI_DialogDrawBackground(dialog->txtbox.x, dialog->txtbox.y,
                                 dialog->txtbox.width, dialog->txtbox.height);
    }
    LCD_SetFontColor(DIALOGBODY_FONT.font_color);
    const char *str = dialog->string_cb
                         ? dialog->string_cb(obj, dialog->cbData)
                         : (const char *)dialog->cbData;
    LCD_GetStringDimensions((const u8 *)str, &dialog->txtbox.width, &dialog->txtbox.height);
    dialog->txtbox.y = box->y + (box->height - dialog->txtbox.height) / 2;
    LCD_PrintStringXY(dialog->txtbox.x, dialog->txtbox.y, str);
}

void GUI_DialogDrawBackground(u16 x, u16 y, u16 w, u16 h)
{
    LCD_FillRect(x, y, w, h, DIALOGBODY_FONT.fill_color);
}

void DialogClose(struct guiObject *obj, u8 state)
{
    struct guiDialog *dialog = &obj->o.dialog;
    void *data = dialog->cbData;
    void (*func)(u8, void*) = dialog->CallBack;
    GUI_RemoveObj(obj);
    func(state, data);
}
void dlgbut_pressok_cb(struct guiObject *obj, void *data)
{
    (void)obj;
    struct guiObject *dlgObj = (struct guiObject *)data;
    DialogClose(dlgObj, 1);
}
void dlgbut_presscancel_cb(struct guiObject *obj, void *data)
{
    (void)obj;
    struct guiObject *dlgObj = (struct guiObject *)data;
    DialogClose(dlgObj, 0);
}
const char * dlgbut_strok_cb(struct guiObject *obj, void *data)
{
    (void)obj;
    (void)data;
    return "Ok";
}
const char * dlgbut_strcancel_cb(struct guiObject *obj, void *data)
{
    (void)obj;
    (void)data;
    return "Cancel";
}
