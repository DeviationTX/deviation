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
#include "pages.h"

#include "../common/_mixer_curves.c"

static u8 action_cb(u32 button, u8 flags, void *data);
static guiObject_t *saveButton;

void MIXPAGE_EditCurves(struct Curve *curve, void *data)
{
    if (curve->type < CURVE_EXPO)
        return;
    PAGE_SetActionCB(action_cb);
    PAGE_RemoveAllObjects();
    edit->parent = (void (*)(void))data;
    edit->pointnum = 0;
    if ((curve->type == CURVE_EXPO || curve->type == CURVE_DEADBAND)
        && curve->points[0] == curve->points[1])
    {
        edit->pointnum = -1;
    }
    edit->curve = *curve;
    edit->curveptr = curve;

    struct LabelDesc labelDesc;
    labelDesc.font = DEFAULT_FONT.font;
    labelDesc.font_color = 0xffff;
    labelDesc.style = LABEL_CENTER;
    labelDesc.outline_color = 0xffff;
    labelDesc.fill_color = labelDesc.outline_color;
    u8 w = 60;
    GUI_CreateTextSelectPlate(0, 0, w, ITEM_HEIGHT, &labelDesc, NULL, set_curvename_cb, NULL);
    u8 x =40;
    w = 40;
    saveButton = GUI_CreateButton(LCD_WIDTH - w, 0, BUTTON_DEVO10, NULL, 0, okcancel_cb, (void *)_tr("Save"));
    GUI_CustomizeButton(saveButton, &labelDesc, w, ITEM_HEIGHT);
    // Draw a line
    GUI_CreateRect(0, ITEM_HEIGHT, LCD_WIDTH, 1, &labelDesc);

    x = 0;
    u8 space = ITEM_HEIGHT + 1;
    u8 y = space;
    w = 65;
    labelDesc.style = LABEL_LEFTCENTER;

    if (curve->type >= CURVE_3POINT) {
        GUI_CreateLabelBox(x, y , w, ITEM_HEIGHT, &labelDesc, NULL, NULL, _tr("Point:"));
        y += space;
        labelDesc.style = LABEL_CENTER;
        edit->pointsel = GUI_CreateTextSelectPlate(x, y, w, ITEM_HEIGHT, &labelDesc, NULL, set_pointnum_cb, NULL);
    } else {
        GUI_CreateLabelBox(x, y , w, ITEM_HEIGHT, &labelDesc, NULL, NULL, _tr("Pos/Neg:"));
        y += space;
        labelDesc.style = LABEL_CENTER;
        edit->pointsel = GUI_CreateTextSelectPlate(x, y, w, ITEM_HEIGHT, &labelDesc, NULL, set_expopoint_cb, NULL);
    }

    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(x, y , w, ITEM_HEIGHT, &labelDesc, NULL, NULL, _tr("Value:"));
    y += space;
    labelDesc.style = LABEL_CENTER;
    edit->value = GUI_CreateTextSelectPlate(x, y, w, ITEM_HEIGHT, &labelDesc, NULL, set_value_cb, NULL);

    edit->graph = GUI_CreateXYGraph(77, ITEM_HEIGHT +1, 50, 50,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, //CHAN_MAX_VALUE / 4, CHAN_MAX_VALUE / 4,
                              show_curve_cb, NULL, touch_cb, &edit->curve);
    GUI_SetSelected(edit->pointsel);
}

static u8 action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            GUI_RemoveAllObjects();
            edit->parent();
        } else if (CHAN_ButtonIsPressed(button, BUT_ENTER)) { // press enter to enter curves setup
            guiObject_t *obj = GUI_GetSelected();
            if (obj != saveButton) {
                GUI_SetSelected(saveButton); // quick jump to save button
            } else
                return 0;
            return 0;
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
