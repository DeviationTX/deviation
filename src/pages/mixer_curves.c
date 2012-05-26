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
#include "pages.h"

static struct curve_edit * const edit = &pagemem.u.mixer_page.edit;
static void okcancel_cb(guiObject_t *obj, void *data);
static const char *set_curvename_cb(guiObject_t *obj, int dir, void *data);
static const char *set_pointnum_cb(guiObject_t *obj, int dir, void *data);
static const char *set_expopoint_cb(guiObject_t *obj, int dir, void *data);
static const char *set_value_cb(guiObject_t *obj, int dir, void *data);
static u8 touch_cb(s16 x, s16 y, void *data);
static s16 show_curve_cb(s16 xval, void *data);

void MIXPAGE_EditCurves(struct Curve *curve, void *data)
{
    if (curve->type < CURVE_EXPO)
        return;
    GUI_RemoveAllObjects();
    edit->parent = (void (*)(void))data;
    edit->pointnum = 0;
    if (curve->type == CURVE_EXPO && curve->points[0] == curve->points[1])
        edit->pointnum = -1;
    edit->curve = *curve;
    edit->curveptr = curve;
    GUI_CreateButton(10, 6, BUTTON_90, "Cancel", 0x0000, okcancel_cb, (void *)0);
    GUI_CreateTextSelect(125, 10, TEXTSELECT_96, 0x0000, NULL, set_curvename_cb, NULL);
    GUI_CreateButton(264, 6, BUTTON_45, "Ok", 0x0000, okcancel_cb, (void *)1);

    if (curve->type >= CURVE_3POINT) {
        GUI_CreateLabel(10, 40, "Point:", 0x0000);
        GUI_CreateTextSelect(10, 60, TEXTSELECT_96, 0x0000, NULL, set_pointnum_cb, NULL);
    } else {
        GUI_CreateLabel(10, 40, "Pos/Neg:", 0x0000);
        GUI_CreateTextSelect(10, 60, TEXTSELECT_96, 0x0000, NULL, set_expopoint_cb, NULL);
    }
    GUI_CreateLabel(10, 86, "Value:", 0x0000);
    edit->value = GUI_CreateTextSelect(10, 106, TEXTSELECT_96, 0x0000, NULL, set_value_cb, NULL);
    edit->graph = GUI_CreateXYGraph(120, 40, 190, 190,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              CHAN_MAX_VALUE / 4, CHAN_MAX_VALUE / 4,
                              show_curve_cb, NULL, touch_cb, &edit->curve);
}

s16 show_curve_cb(s16 xval, void *data)
{
    (void)data;
    s16 oldpoint;
    s16 yval;
    if (edit->pointnum < 0) {
        oldpoint = edit->curve.points[1];
        edit->curve.points[1] = edit->curve.points[0];
    }
    yval = CURVE_Evaluate(xval, &edit->curve);
    if (edit->pointnum < 0) {
        edit->curve.points[1] = oldpoint;
    }
    return yval;
}
    
static const char *set_curvename_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    struct Curve *curve = &edit->curve;
    u8 changed;
    if (curve->type >= CURVE_3POINT) {
        curve->type = GUI_TextSelectHelper(curve->type, CURVE_3POINT, CURVE_MAX, dir, 1, 1, &changed);
        if (changed)
            GUI_Redraw(edit->graph);
    }
    return CURVE_GetName(curve);
}
static void okcancel_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    if (data) {
        if (edit->pointnum < 0)
            edit->curve.points[1] = edit->curve.points[0];
        *edit->curveptr = edit->curve;
    }
    GUI_RemoveAllObjects();
    edit->parent();
}
static const char *set_value_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    struct Curve *curve = &edit->curve;
    u8 pointnum = edit->pointnum < 0 ? 0 : edit->pointnum;
    s8 old_pointval = curve->points[pointnum];
    const char *ret = PAGEMIX_SetNumberCB(obj, dir, &curve->points[pointnum]);
    if (old_pointval != curve->points[pointnum]) {
        GUI_Redraw(edit->graph);
        if (edit->pointnum < 0)
            edit->curve.points[1] = edit->curve.points[0];
    }
    return ret;
}
   
static const char *set_pointnum_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    struct Curve *curve = &edit->curve;
    if (curve->type >= CURVE_3POINT) {
        u8 changed;
        edit->pointnum = GUI_TextSelectHelper(edit->pointnum, 0, (curve->type - CURVE_3POINT) * 2 + 2, dir, 1, 1, &changed);
        if (changed)
            GUI_Redraw(edit->value);
    }
    switch(edit->pointnum) {
        case 0: return "0";
        case 1: return "1";
        case 2: return "2";
        case 3: return "3";
        case 4: return "4";
        case 5: return "5";
        case 6: return "6";
        case 7: return "7";
        case 8: return "8";
        case 9: return "9";
        case 10: return "10";
        case 11: return "11";
        case 12: return "12";
        default: return "0";
    }
}

const char *set_expopoint_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;

    u8 changed;
    edit->pointnum = GUI_TextSelectHelper(edit->pointnum, -1, 1, dir, 1, 1, &changed);
    if (changed) {
        GUI_Redraw(edit->value);
        GUI_Redraw(edit->graph);
    }
    switch(edit->pointnum) {
        case -1: return "Symmetric";
        case 0: return "Pos";
        case 1: return "Neg";
        default: return "-";
    }
}
static u8 touch_cb(s16 x, s16 y, void *data)
{
    (void)data;
    (void)x;
    u8 pointnum = edit->pointnum < 0 ? 0 : edit->pointnum;
    edit->curve.points[pointnum] = RANGE_TO_PCT(y);
    GUI_Redraw(edit->value);
    return 1;
}
