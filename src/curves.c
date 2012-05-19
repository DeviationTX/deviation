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
    int num_points = (curve->type - CURVE_3POINT) * 2 + 2;
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
