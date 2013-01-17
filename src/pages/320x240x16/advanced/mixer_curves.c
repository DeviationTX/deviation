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

void MIXPAGE_EditCurves(struct Curve *curve, void *data)
{
    if (curve->type < CURVE_EXPO)
        return;
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
    GUI_CreateTextSelect(&gui->name, 8, 8, TEXTSELECT_96, 0x0000, NULL, set_curvename_cb, NULL);
    PAGE_CreateCancelButton(160, 4, okcancel_cb);
    PAGE_CreateOkButton(264, 4, okcancel_cb);

    if (curve->type >= CURVE_3POINT) {
        GUI_CreateLabel(&gui->pointlbl, 8, 40, NULL, DEFAULT_FONT, _tr("Point:"));
        GUI_CreateTextSelect(&gui->point, 8, 56, TEXTSELECT_96, 0x0000, NULL, set_pointnum_cb, NULL);
    } else {
        GUI_CreateLabel(&gui->pointlbl, 8, 40, NULL, DEFAULT_FONT, _tr("Pos/Neg:"));
        GUI_CreateTextSelect(&gui->point, 8, 56, TEXTSELECT_96, 0x0000, NULL, set_expopoint_cb, NULL);
    }
    GUI_CreateLabel(&gui->valuelbl, 8, 80, NULL, DEFAULT_FONT, _tr("Value:"));
    GUI_CreateTextSelect(&gui->value, 8, 96, TEXTSELECT_96, 0x0000, NULL, set_value_cb, NULL);
    GUI_CreateXYGraph(&gui->graph, 112, 36, 200, 200,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              CHAN_MAX_VALUE / 4, CHAN_MAX_VALUE / 4,
                              show_curve_cb, NULL, touch_cb, &edit->curve);
}
