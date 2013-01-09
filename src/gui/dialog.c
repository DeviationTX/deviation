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

#include "_dialog.c"

static void dlgbut_pressok_cb(struct guiObject *obj, const void *data);
static void dlgbut_presscancel_cb(struct guiObject *obj, const void *data);
const char *dlgbut_strok_cb(struct guiObject *obj, const void *data);
const char *dlgbut_strcancel_cb(struct guiObject *obj, const void *data);

guiObject_t *GUI_CreateDialog(guiDialog_t *dialog, u16 x, u16 y, u16 width, u16 height, const char *title,
        const char *(*string_cb)(guiObject_t *obj, void *data),
        void (*CallBack)(u8 state, void *data),
        enum DialogType dgType, void *data)
{
    struct guiHeader *obj = (guiObject_t *)dialog;
    struct guiBox *box ;

    box = &obj->box;

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

    struct guiObject *but = NULL;
    int button_width  = GUI_ButtonWidth(DIALOG_BUTTON);
    int button_height = GUI_ButtonHeight(DIALOG_BUTTON);
    switch (dgType) {
    case dtOk: {
        but = GUI_CreateButton(&dialog->but1, x + (width - button_width) / 2, y + height - button_height - 1,
                    DIALOG_BUTTON, dlgbut_strok_cb, 0x0000, dlgbut_pressok_cb, obj);
        OBJ_SET_MODAL(but, 1);
        }
        break;
    case dtOkCancel: {
        GUI_CreateButton(&dialog->but1, x + (width - button_width - button_width) / 2, y + height - button_height - 1,
                DIALOG_BUTTON, dlgbut_strok_cb, 0x0000, dlgbut_pressok_cb, obj);
        OBJ_SET_MODAL(but, 1);
        but = GUI_CreateButton(&dialog->but2, x + width/2, y + height - button_height - 1,
                 DIALOG_BUTTON, dlgbut_strcancel_cb, 0x0000, dlgbut_presscancel_cb, obj);
        OBJ_SET_MODAL(but, 1);
        }
        break;
    case dtNone:
        break;
    }
    GUI_HandleModalButtons(1);
    objSELECTED = but;
    //bug fix: using objSELECTED for dialog is not safe in devo10
    objModalButton = but;
    return obj;
}

void GUI_DrawDialog(struct guiObject *obj)
{
    struct guiBox *box = &obj->box;
    struct guiDialog *dialog = (struct guiDialog *)dialog;
    if (dialog->txtbox.height == 0) {
        _draw_dialog_box(box, dialog->txtbox.x, dialog->title);
    } else if(dialog->txtbox.width) {
        // NOTE: We assume all redraw events after the 1st are incremental!
        GUI_DialogDrawBackground(dialog->txtbox.x, dialog->txtbox.y,
                                 dialog->txtbox.width, dialog->txtbox.height);
    }
    const char *str = dialog->string_cb
                         ? dialog->string_cb(obj, dialog->cbData)
                         : (const char *)dialog->cbData;
    int button_height = GUI_ButtonHeight(DIALOG_BUTTON);
    LCD_SetFont(DIALOGBODY_FONT.font);
    LCD_SetFontColor(DIALOGBODY_FONT.font_color);
    LCD_GetStringDimensions((const u8 *)str, &dialog->txtbox.width, &dialog->txtbox.height);
    dialog->txtbox.y = box->y + DIALOG_HEADER_Y +
        (box->height - dialog->txtbox.height - DIALOG_HEADER_Y - button_height) / 2;
    LCD_PrintStringXY(dialog->txtbox.x, dialog->txtbox.y, str);
}

void GUI_DialogDrawBackground(u16 x, u16 y, u16 w, u16 h)
{
    _dialog_draw_background(x, y, w, h);
}

void DialogClose(struct guiObject *obj, u8 state)
{
    struct guiDialog *dialog = (struct guiDialog *)obj;
    void *data = dialog->cbData;
    void (*func)(u8, void*) = dialog->CallBack;
    GUI_RemoveObj(obj);
    func(state, data);
}
void dlgbut_pressok_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    struct guiObject *dlgObj = (struct guiObject *)data;
    DialogClose(dlgObj, 1);
}

void dlgbut_presscancel_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    struct guiObject *dlgObj = (struct guiObject *)data;
    DialogClose(dlgObj, 0);
}

const char * dlgbut_strok_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    (void)data;
    return _tr("Ok");
}
const char * dlgbut_strcancel_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    (void)data;
    return _tr("Cancel");
}
