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

static struct advcurve_obj * const gui  = &gui_objs.u.advcurve;
static struct curve_edit   * const edit = &pagemem.u.mixer_page.edit;

static void okcancel_cb(guiObject_t *obj, const void *data);
static const char *set_curvename_cb(guiObject_t *obj, int dir, void *data);
static const char *set_pointnum_cb(guiObject_t *obj, int dir, void *data);
static const char *set_expopoint_cb(guiObject_t *obj, int dir, void *data);
static const char *set_value_cb(guiObject_t *obj, int dir, void *data);
static u8 touch_cb(s16 x, s16 y, void *data);
static s32 show_curve_cb(s32 xval, void *data);

s32 show_curve_cb(s32 xval, void *data)
{
    (void)data;
    s16 oldpoint;
    s32 yval;
    if (edit->reverse)
        xval = -xval; 
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
    u8 type = CURVE_TYPE(curve);
    if (type >= CURVE_3POINT) {
        type = GUI_TextSelectHelper(type, CURVE_3POINT, CURVE_MAX, dir, 1, 1, &changed);
        if (changed) {
            CURVE_SET_TYPE(curve, type);
            GUI_Redraw(&gui->graph);
            GUI_Redraw(&gui->point);
        }
    }
    return CURVE_GetName(tempstring, curve);
}
static void okcancel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    if (data) {
        if (edit->pointnum < 0)
            edit->curve.points[1] = edit->curve.points[0];
        *edit->curveptr = edit->curve;
    }
    PAGE_Pop();
}
static const char *set_value_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    const char *ret;
    struct Curve *curve = &edit->curve;
    u8 pointnum = edit->pointnum < 0 ? 0 : edit->pointnum;
    s8 old_pointval = curve->points[pointnum];
    if(CURVE_TYPE(curve) == CURVE_DEADBAND) {
        curve->points[pointnum] = (s8)GUI_TextSelectHelper((u8)curve->points[pointnum], 0, 255, dir, 1, 5, NULL);
        sprintf(tempstring, "%d.%d", (u8)curve->points[pointnum] / 10, (u8)curve->points[pointnum] % 10);
        ret = tempstring;
    } else {
        ret = PAGEMIXER_SetNumberCB(obj, dir, &curve->points[pointnum]);
    }
    if (old_pointval != curve->points[pointnum]) {
        GUI_Redraw(&gui->graph);
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
    if (CURVE_TYPE(curve) >= CURVE_3POINT) {
        u8 changed;
        u8 max = (CURVE_TYPE(curve) - CURVE_3POINT) * 2 + 2;
        edit->pointnum = GUI_TextSelectHelper(edit->pointnum, 0, max, dir, 1, 1, &changed);
        if (changed)
            GUI_Redraw(&gui->value);
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
        GUI_Redraw(&gui->value);
        GUI_Redraw(&gui->graph);
    }
    switch(edit->pointnum) {
        case -1: return _tr("Symmetric");
        case 0: return _tr("Pos");
        case 1: return _tr("Neg");
        default: return "-";
    }
}

const char *set_smooth_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;

    u8 changed;
    u8 smooth = CURVE_SMOOTHING(&edit->curve) ? 1 : 0;
    smooth = GUI_TextSelectHelper(smooth, 0, 1, dir, 1, 1, &changed);
    if (changed) {
        CURVE_SET_SMOOTHING(&edit->curve, smooth);
        GUI_Redraw(&gui->value);
        GUI_Redraw(&gui->graph);
    }
    return smooth ? _tr("Yes") : _tr("No");
}

static u8 touch_cb(s16 x, s16 y, void *data)
{
    (void)data;
    (void)x;
    u8 pointnum = edit->pointnum < 0 ? 0 : edit->pointnum;
    if (CURVE_TYPE(&edit->curve) < CURVE_EXPO) {
        edit->curve.points[pointnum] = RANGE_TO_PCT(x);
    } else {
        if (CURVE_TYPE(&edit->curve) >= CURVE_3POINT) {
            u8 maxpt = (CURVE_TYPE(&edit->curve) - CURVE_3POINT) * 2 + 2;
            if (edit->reverse)
                x = -x;
            x -= CHAN_MIN_VALUE;
            int delta = (CHAN_MAX_VALUE - CHAN_MIN_VALUE) / maxpt;
            int point = (x + delta / 2) / delta;
            if (point != pointnum) {
                edit->pointnum = point;
printf("Setting point from %d -> %d\n", edit->pointnum, point);
                GUI_Redraw(&gui->point);
            }
            pointnum = point;
        }
        edit->curve.points[pointnum] = RANGE_TO_PCT(y);
    }
    GUI_Redraw(&gui->value);
    return 1;
}
