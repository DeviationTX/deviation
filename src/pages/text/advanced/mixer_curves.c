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
#include "../pages.h"

#include "../../common/advanced/_mixer_curves.c"

static unsigned action_cb(u32 button, unsigned flags, void *data);

void MIXPAGE_EditCurves(struct Curve *curve, void *data)
{
    u8 type = CURVE_TYPE(curve);
    if (type < CURVE_FIXED)
        return;
    GUI_SelectionNotify(NULL);
    PAGE_SetActionCB(action_cb);
    PAGE_RemoveAllObjects();
    edit->parent = (void (*)(void))data;
    edit->pointnum = 0;
    edit->reverse = MIXER_SRC_IS_INV(pagemem.u.mixer_page.cur_mixer->src);
    if ((type == CURVE_EXPO || type == CURVE_DEADBAND)
        && curve->points[0] == curve->points[1])
    {
        edit->pointnum = -1;
    }
    edit->curve = *curve;
    edit->curveptr = curve;

    labelDesc.style = LABEL_CENTER;
    u8 w = 60;
    GUI_CreateTextSelectPlate(&gui->name, 0, 0, w, ITEM_HEIGHT, &labelDesc, NULL, set_curvename_cb, NULL);
    u8 x =40;
    w = 40;
    GUI_CreateButtonPlateText(&gui->save, LCD_WIDTH - w, 0, w, ITEM_HEIGHT,&labelDesc , NULL, 0, okcancel_cb, (void *)_tr("Save"));
    // Draw a line
    GUI_CreateRect(&gui->rect, 0, ITEM_HEIGHT, LCD_WIDTH, 1, &labelDesc);

    x = 0;
    u8 space = ITEM_HEIGHT + 1;
    u8 y = space;
    w = 74;
    labelDesc.style = LABEL_LEFTCENTER;

    if (type >= CURVE_3POINT) {
        GUI_CreateLabelBox(&gui->smoothlbl, x, y, w-35, ITEM_HEIGHT, &labelDesc, NULL, NULL, _tr("Smooth"));
        GUI_CreateTextSelectPlate(&gui->smooth, x + w - 35, y, 35, ITEM_HEIGHT, &labelDesc, NULL, set_smooth_cb, NULL);
        y += space;
        GUI_CreateLabelBox(&gui->pointlbl, x, y , w-24, ITEM_HEIGHT, &labelDesc, NULL, NULL, _tr("Point"));
        GUI_CreateTextSelectPlate(&gui->point, x + w - 24, y, 24, ITEM_HEIGHT, &TINY_FONT, NULL, set_pointnum_cb, NULL);
    } else if(type == CURVE_DEADBAND || type == CURVE_EXPO) {
        GUI_CreateLabelBox(&gui->pointlbl, x, y , w, ITEM_HEIGHT, &labelDesc, NULL, NULL, _tr("Pos/Neg"));
        y += space;
        labelDesc.style = LABEL_CENTER;
        GUI_CreateTextSelectPlate(&gui->point, x, y, w, ITEM_HEIGHT, &labelDesc, NULL, set_expopoint_cb, NULL);
    }

    y += space;
    labelDesc.style = LABEL_LEFTCENTER;
    GUI_CreateLabelBox(&gui->valuelbl, x, y , w, ITEM_HEIGHT, &labelDesc, NULL, NULL, _tr("Value"));
    y += space;
    labelDesc.style = LABEL_CENTER;
    GUI_CreateTextSelectPlate(&gui->value, x, y, w, ITEM_HEIGHT, &labelDesc, NULL, set_value_cb, NULL);

    GUI_CreateXYGraph(&gui->graph, 77, ITEM_HEIGHT +1, 50, 50,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, //CHAN_MAX_VALUE / 4, CHAN_MAX_VALUE / 4,
                              show_curve_cb, NULL, touch_cb, &edit->curve);
    GUI_SetSelected((guiObject_t *)&gui->point);
}

static unsigned action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_RemoveAllObjects();
            edit->parent();
        } else if (CHAN_ButtonIsPressed(button, BUT_ENTER) && (flags & BUTTON_LONGPRESS)) {
            // long press enter = save without exiting
            if (edit->pointnum < 0)
                edit->curve.points[1] = edit->curve.points[0];
            *edit->curveptr = edit->curve;
            struct mixer_page * const mp = &pagemem.u.mixer_page;
            PAGE_SaveMixerSetup(mp);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
