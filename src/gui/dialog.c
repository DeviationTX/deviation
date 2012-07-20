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
#include "config/display.h"

static void dgCallback(struct guiObject *obj, void *data);

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
    dialog->file = "media/dialog.bmp";

    obj->Type = Dialog;
    OBJ_SET_TRANSPARENT(obj, LCD_ImageIsTransparent(dialog->file));
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
                ((y + height) - 27), BUTTON_96, "Ok", 0x0000, dgCallback, obj);
        OBJ_SET_MODAL(dialog->button[0], 1);
        break;
    case dtOkCancel:
        break;
    }

    return obj;

}

void GUI_DrawDialog(struct guiObject *obj)
{
    struct guiBox *box = &obj->box;
    struct guiDialog *dialog = &obj->o.dialog;
    //printf("Draw Dialog: X: %d Y: %d WIDTH: %d HEIGHT: %d\n", box->x,
    //        box->y, box->width, box->height);
    LCD_DrawWindowedImageFromFile(box->x, box->y, dialog->file,
            box->width, box->height, 0, 0);
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
}

static void dgCallback(struct guiObject *obj, void *data)
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

