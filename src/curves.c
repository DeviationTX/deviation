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
#include "mixer.h"
#include "gui/gui.h"

/*
    {-100, 0, 100},
    {-100, -50, 0, 50, 100},
    {-100, -67, -33, 0, 33, 67, 100},
    {-100, -75, -50, -25, 0 25, 50, 75, 100},
    {-100, -80, -60, -40, -20, 0, 20, 40, 60, 80, 100},
    {-100, -83, -67, -50, -33, -17, 0, 17, 33, 50, 67, 83, 100}
*/

s16 interpolate(struct Curve *curve, s16 value)
{
    int i;
    int num_points = (curve->type - CURVE_3POINT) * 2 + 3;
    int step = 2 * 10000 / (num_points - 1) ;
    for (i = 0; i < num_points - 1; i++) {
        s16 x = -10000 + i * step;
        s16 pos1 = PCT_TO_RANGE(x / 100);
        s16 pos2 = PCT_TO_RANGE((x + step) / 100);
        if(value >= pos1 && value <= pos2) {
            s32 tmp = ((s32)value - pos1) * (curve->points[i + 1] - curve->points[i]) / (pos2 - pos1) + curve->points[i];
            return PCT_TO_RANGE(tmp);
        }
    }
    return PCT_TO_RANGE(curve->points[num_points - 1]);
}


s16 CURVE_Evaluate(s16 xval, struct Curve *curve)
{
    switch (curve->type) {
        case CURVE_NONE:     return xval;
        case CURVE_MIN_MAX:  return (xval < 0) ? CHAN_MIN_VALUE : CHAN_MAX_VALUE;
        case CURVE_ZERO_MAX: return (xval < 0) ? 0 : CHAN_MAX_VALUE;
        case CURVE_GT_ZERO:  return (xval < 0) ? 0 : xval;
        case CURVE_LT_ZERO:  return (xval > 0) ? 0 : xval;
        case CURVE_ABSVAL:   return (xval < 0) ? -xval : xval;
        default:             return interpolate(curve, xval);
    }
}

const char *CURVE_GetName(struct Curve *curve)
{
    switch (curve->type) {
        case CURVE_NONE: return "None";
        case CURVE_MIN_MAX:  return "Min/Max";
        case CURVE_ZERO_MAX: return "Zero/Max";
        case CURVE_GT_ZERO:  return "> 0";
        case CURVE_LT_ZERO:  return "< 0";
        case CURVE_ABSVAL:   return "ABSVAL";
        case CURVE_3POINT:   return "3 Point";
        case CURVE_5POINT:   return "5 Point";
        case CURVE_7POINT:   return "7 Point";
        case CURVE_9POINT:   return "9 Point";
        case CURVE_11POINT:  return "11 Point";
        case CURVE_13POINT:  return "13 Point";
    }
    return "Unknown";
}

static void okcancel_cb(guiObject_t *obj, void *data);
static const char *set_curvename_cb(guiObject_t *obj, int dir, void *data);
static const char *set_pointnum_cb(guiObject_t *obj, int dir, void *data);
static const char *set_value_cb(guiObject_t *obj, int dir, void *data);
static u8 touch_cb(s16 x, s16 y, void *data);

struct curve_edit {
    struct Curve curve;
    struct Curve *curveptr;
    void(*parent)(void);
    u8 pointnum;
    guiObject_t *graph;
};
static struct curve_edit edit;


void CURVE_Edit(struct Curve *curve, void *data)
{
    if (curve->type < CURVE_3POINT)
        return;
    GUI_RemoveAllObjects();
    edit.parent = (void (*)(void))data;
    edit.pointnum = 0;
    edit.curve = *curve;
    edit.curveptr = curve;
    GUI_CreateButton(10, 6, BUTTON_90, "Cancel", 0x0000, okcancel_cb, (void *)0);
    GUI_CreateTextSelect(125, 10, TEXTSELECT_96, 0x0000, NULL, set_curvename_cb, NULL);
    GUI_CreateButton(264, 6, BUTTON_45, "Ok", 0x0000, okcancel_cb, (void *)1);

    GUI_CreateLabel(10, 40, "Point:", 0x0000);
    GUI_CreateTextSelect(10, 60, TEXTSELECT_96, 0x0000, NULL, set_pointnum_cb, NULL);

    GUI_CreateLabel(10, 86, "Value:", 0x0000);
    GUI_CreateTextSelect(10, 106, TEXTSELECT_96, 0x0000, NULL, set_value_cb, NULL);
    edit.graph = GUI_CreateXYGraph(120, 40, 190, 190,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              (s16 (*)(s16,  void *))CURVE_Evaluate, touch_cb, &edit.curve);
}

static const char *set_curvename_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    struct Curve *curve = &edit.curve;
    if(dir > 0 && curve->type < CURVE_MAX) {
        curve->type++;
        GUI_Redraw(edit.graph);
    } else if(dir < 0 && curve->type > CURVE_3POINT) {
        curve->type--;
        GUI_Redraw(edit.graph);
    }
    return CURVE_GetName(curve);
}
static void okcancel_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    if (data)
        *edit.curveptr = edit.curve;
    GUI_RemoveAllObjects();
    edit.parent();
}
static const char *set_value_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    struct Curve *curve = &edit.curve;
    static char str[5];
    if (dir > 0) {
        if (curve->points[edit.pointnum] < 100) {
            curve->points[edit.pointnum]++;
            GUI_Redraw(edit.graph);
        }
    } else if (dir < 0) {
        if (curve->points[edit.pointnum] > -100) {
            curve->points[edit.pointnum]--;
            GUI_Redraw(edit.graph);
        }
    }
    sprintf(str, "%d", curve->points[edit.pointnum]);
    return str;
}
   
static const char *set_pointnum_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    struct Curve *curve = &edit.curve;
    if(dir > 0) {
        if(edit.pointnum < (curve->type - CURVE_3POINT) * 2 + 2)
            edit.pointnum++;
    } else if(dir < 0) {
        if(edit.pointnum)
            edit.pointnum--;
    }
    switch(edit.pointnum) {
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

static u8 touch_cb(s16 x, s16 y, void *data)
{
    (void)data;
    (void)x;
    edit.curve.points[edit.pointnum] = RANGE_TO_PCT(y);
    return 1;
}
