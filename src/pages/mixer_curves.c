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
    PAGE_RemoveAllObjects();
    edit->parent = (void (*)(void))data;
    edit->pointnum = 0;
    if (curve->type == CURVE_EXPO && curve->points[0] == curve->points[1])
        edit->pointnum = -1;
    edit->curve = *curve;
    edit->curveptr = curve;
    GUI_CreateTextSelect(8, 8, TEXTSELECT_96, 0x0000, NULL, set_curvename_cb, NULL);
    PAGE_CreateCancelButton(160, 4, okcancel_cb);
    PAGE_CreateOkButton(264, 4, okcancel_cb);

    if (curve->type >= CURVE_3POINT) {
        GUI_CreateLabel(8, 40, NULL, DEFAULT_FONT, _tr("Point:"));
        edit->pointsel = GUI_CreateTextSelect(8, 56, TEXTSELECT_96, 0x0000, NULL, set_pointnum_cb, NULL);
    } else {
        GUI_CreateLabel(8, 40, NULL, DEFAULT_FONT, _tr("Pos/Neg:"));
        edit->pointsel = GUI_CreateTextSelect(8, 56, TEXTSELECT_96, 0x0000, NULL, set_expopoint_cb, NULL);
    }
    GUI_CreateLabel(8, 80, NULL, DEFAULT_FONT, _tr("Value:"));
    edit->value = GUI_CreateTextSelect(8, 96, TEXTSELECT_96, 0x0000, NULL, set_value_cb, NULL);
    edit->graph = GUI_CreateXYGraph(112, 36, 200, 200,
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
        yval = CURVE_Evaluate(xval, &edit->curve);
        edit->curve.points[1] = oldpoint;
    } else {
        yval = CURVE_Evaluate(xval, &edit->curve);
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
        if (changed) {
            GUI_Redraw(edit->graph);
            GUI_Redraw(edit->pointsel);
        }
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
    const char *ret = PAGEMIXER_SetNumberCB(obj, dir, &curve->points[pointnum]);
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
        u8 max = (curve->type - CURVE_3POINT) * 2 + 2;
        edit->pointnum = GUI_TextSelectHelper(edit->pointnum, 0, max, dir, 1, 1, &changed);
        if (changed)
            GUI_Redraw(edit->value);
    }
    switch(edit->pointnum) {
        case 0: return "1";
        case 1: return "2";
        case 2: return "3";
        case 3: return "4";
        case 4: return "5";
        case 5: return "6";
        case 6: return "7";
        case 7: return "8";
        case 8: return "9";
        case 9: return "10";
        case 10: return "11";
        case 11: return "12";
        case 12: return "13";
        default: return "1";
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
        case -1: return _tr("Symmetric");
        case 0: return _tr("Pos");
        case 1: return _tr("Neg");
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
