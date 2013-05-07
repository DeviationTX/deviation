#include "common.h"
#include "mixer.h"

#include <stdlib.h>

/*
    {-100, 0, 100},
    {-100, -50, 0, 50, 100},
    {-100, -67, -33, 0, 33, 67, 100},
    {-100, -75, -50, -25, 0 25, 50, 75, 100},
    {-100, -80, -60, -40, -20, 0, 20, 40, 60, 80, 100},
    {-100, -83, -67, -50, -33, -17, 0, 17, 33, 50, 67, 83, 100}
*/

s32 compute_ctrl_pt(struct Curve *curve, int num_points, int i, s32 vx, s32 vy, s32 px)
{
    s32 m, b;
    s32 delta = 2 * 100 / (num_points - 1);
    #define MMULT 1024
    if (i == 0) {
        //linear interpolation between 1st 2 points
        //keep 3 decimal-places for m
        m = (MMULT * (curve->points[i+1] - curve->points[i])) / delta;
    } else if (i == num_points - 1) {
        //linear interpolation between last 2 points
        //keep 3 decimal-places for m
        m = (MMULT * (curve->points[i] - curve->points[i-1])) / delta;
    } else {
        //use slope between i-1 and i+1 points (roughly dy/dx)
        m = (MMULT * (curve->points[i+1] - curve->points[i-1])) / (2*delta);
        //ensure the smoothed value never overshoots the data points
        //This is done by ensuring that no control point has a max-y larger
        //Than any of the points: i-1, i, i+1
        s32 delta_y =  abs((px - vx) * m / MMULT);
        s32 max = (curve->points[i+1] > curve->points[i]) ? curve->points[i+1] : curve->points[i];
        max = PCT_TO_RANGE((max > curve->points[i-1]) ? max : curve->points[i-1]);
        s32 min = (curve->points[i+1] < curve->points[i]) ? curve->points[i+1] : curve->points[i];
        min = PCT_TO_RANGE((min < curve->points[i-1]) ? min : curve->points[i-1]);
        //in the case that the control-point y would be too large, truncate it to the max value
        if (delta_y + vy > max) {
            m = m * (max - vy) / delta_y;
        } else if (vy - delta_y < min) {
            m = m * (vy - min) / delta_y;
        }
    }
    b = vy - m * vx / MMULT;
    s32 py = m * px/ MMULT + b;
    return py;
}

//Cubic Bezier Equation is:
//C(t) = (1-t)^3*P0 + (1-t)^2*t*P1 + (1-t)*t^2*P2 + t^3*P3
//where 0 <= t <= 1 and represents the current position between the endpoints
//P0 and P3 are the end-points
//P1 and P2 are the control points
//We use symetric control points around each endpoint for a smooth curve
//The BEZIER_FACTOR is used to control the magnitude fo the control-points
//The smaller the factor the more impact the control-points will have
#define BEZIER_FACTOR 5

s16 bezier_spline(struct Curve *curve, s32 value)
{
    int num_points = (CURVE_TYPE(curve) - CURVE_3POINT) * 2 + 3;
    s32 step = PCT_TO_RANGE(2 * 100 / (num_points - 1)) ;
    for (int i = 0; i < num_points -1; i++) {
        s32 x = PCT_TO_RANGE(-100) + i * step;
        s32 p0x = x;
        s32 p3x = x + step;
        if(value >= p0x && value <= p3x) {
            s32 p0y = PCT_TO_RANGE(curve->points[i]);
            s32 p3y = PCT_TO_RANGE(curve->points[i+1]);
            s32 delta = (p3x - p0x) / BEZIER_FACTOR; 
            s32 p1y, p2y;

            /* Start without the cache */
#if 0
            u8 needs_px = 3;
            // index is offset by 16 to ensure it is computed the 1st time
            if ((i << 4) != curve->index) {
                if ((i << 4) == curve->index + 1) {
                    //move p1 -> p2
                    curve->p2 = curve->points[i+1] - (curve->p1 - curve->points[i+1]);
                    needs_px = 1;
                } else if ((i << 4) + 1 == curve->index) {
                    //move p3 -> p1
                    curve->p1 = curve->points[i] - (curve->p2 - curve->points[i]);
                    needs_px = 2;
                }
                curve->index = i << 4;
            }

            if (needs_px & 0x01) {
                s32 p1x = p0x + delta;
                p1y = compute_ctrl_pt(curve, num_points, i, p0x, p0y, p1x);
                curve->p1 = RANGE_TO_PCT(p1y);
            } else {
                p1y = PCT_TO_RANGE(curve->p1);
            }
            if (needs_px & 0x02) {
                s32 p2x = p3x - delta;
                p2y = compute_ctrl_pt(curve, num_points, i+1, p3x, p3y, p2x);
                curve->p2 = RANGE_TO_PCT(p2y);
            } else {
                p2y = PCT_TO_RANGE(curve->p2);
            }
#else 
            s32 p1x = p0x + delta;
            p1y = compute_ctrl_pt(curve, num_points, i, p0x, p0y, p1x);
            s32 p2x = p3x - delta;
            p2y = compute_ctrl_pt(curve, num_points, i+1, p3x, p3y, p2x);
#endif
            #define TMULT 16536
            u32 t = (TMULT * (value - p0x)) / (p3x - p0x);
            //printf("%d: P0(%d, %d) P1(%d, %d) P2(%d, %d) P3(%d, %d)\n",
            //       t, p0x, p0y, p1x, p1y, p2x, p2y, p3x, p3y);
            u32 t_2 = t * t / TMULT;
            u32 t_3 = t_2 * t / TMULT;
            u32 t1 = TMULT - t;
            u32 t1_2 = t1 * t1 / TMULT;
            u32 t1_3 = t1_2 * t1 / TMULT;
            //printf("t: %d t^2: %d t^3: %d     t-1: %d (t-1)^2: %d (t-1)^3: %d\n",
            //       t, t_2, t_3, t1, t1_2, t1_3);
            s32 value = ((t1_3 * p0y)
                         + (3 * t1_2 * t / TMULT * p1y)
                         + (3 * t1 * t_2 / TMULT * p2y)
                         + (t_3 * p3y)
                        );
            value /= TMULT;
            //printf("x0: %d y1: %d x2: %d x3: %d => %d\n",
            //       t1_3 * p0y, 3 * t1_2 * t / 1000 * p1y, 3 * t1 * t_2 / 1000 * p2y, t_3 * p3y, value);
            return value;
        }
    }
    return 0;
}

s16 interpolate(struct Curve *curve, s32 value)
{
    int i;
    int num_points = (CURVE_TYPE(curve) - CURVE_3POINT) * 2 + 3;
    s32 step = 2 * 10000 / (num_points - 1) ;
    for (i = 0; i < num_points - 1; i++) {
        s32 x = -10000 + i * step;
        s32 pos1 = PCT_TO_RANGE(x / 100);
        s32 pos2 = PCT_TO_RANGE((x + step) / 100);
        if(value >= pos1 && value <= pos2) {
            s32 tmp = (value - pos1) * (curve->points[i + 1] - curve->points[i]) / (pos2 - pos1) + curve->points[i];
            return PCT_TO_RANGE(tmp);
        }
    }
    return PCT_TO_RANGE(curve->points[num_points - 1]);
}

/* This camefrom er9x/th9x
 * expo-funktion:
 * ---------------
 * kmplot
 * f(x,k)=exp(ln(x)*k/10) ;P[0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20]
 * f(x,k)=x*x*x*k/10 + x*(1-k/10) ;P[0,1,2,3,4,5,6,7,8,9,10]
 * f(x,k)=x*x*k/10 + x*(1-k/10) ;P[0,1,2,3,4,5,6,7,8,9,10]
 * f(x,k)=1+(x-1)*(x-1)*(x-1)*k/10 + (x-1)*(1-k/10) ;P[0,1,2,3,4,5,6,7,8,9,10]
 */

s16 expou(u32 x, u16 k)
{
    // k*x*x*x + (1-k)*x
    // 0 <= k <= 100
    #define KMAX 100
    u32 val = (x * x / CHAN_MAX_VALUE * x / CHAN_MAX_VALUE * k
               + (KMAX - k) * x + KMAX / 2) / KMAX;
    return val;
}
s16 expo(struct Curve *curve, s32 value)
{

    s32  y;
    s32 k;
    u8 neg = value < 0;

    k = neg ? curve->points[1] : curve->points[0];
    if (k == 0)
        return value;

    if (neg)
        value = -value; //absval

    if (k < 0) {
        y = CHAN_MAX_VALUE - expou(CHAN_MAX_VALUE - value, -k);
    }else{
        y = expou(value, k);
    }
    return neg ? -y : y;
}

s16 deadband(struct Curve *curve, s32 value)
{
    u8 neg = value < 0;
    s32 k = neg ? (u8)curve->points[1] : (u8)curve->points[0];

    if (k == 0)
        return CHAN_MAX_VALUE;
    value = abs(value);
    if (value < k * CHAN_MULTIPLIER / 10)
        return 0;
    s32 max = CHAN_MAX_VALUE;
    return max * ((1000 * (value - max) + (1000 - k) * max) / (1000 - k)) / value;
}

s16 CURVE_Evaluate(s16 xval, struct Curve *curve)
{
    //interpolation doesn't work if theinput is out of bounds, so bound it here
    if (xval > CHAN_MAX_VALUE)
        xval = CHAN_MAX_VALUE;
    else if (xval < CHAN_MIN_VALUE)
        xval = CHAN_MIN_VALUE;
    switch (CURVE_TYPE(curve)) {
        case CURVE_NONE:     return xval;
        case CURVE_FIXED:    return CHAN_MAX_VALUE;
        case CURVE_MIN_MAX:  return (xval < 0) ? CHAN_MIN_VALUE : CHAN_MAX_VALUE;
        case CURVE_ZERO_MAX: return (xval < 0) ? 0 : CHAN_MAX_VALUE;
        case CURVE_GT_ZERO:  return (xval < 0) ? 0 : xval;
        case CURVE_LT_ZERO:  return (xval > 0) ? 0 : xval;
        case CURVE_ABSVAL:   return (xval < 0) ? -xval : xval;
        case CURVE_EXPO:     return expo(curve, xval);
        case CURVE_DEADBAND: return deadband(curve, xval);
        default:             return CURVE_SMOOTHING(curve)
                                    ? bezier_spline(curve, xval)
                                    : interpolate(curve, xval);
    }
}

const char *CURVE_GetName(struct Curve *curve)
{
    switch (CURVE_TYPE(curve)) {
        case CURVE_NONE: return _tr("1-to-1");
        case CURVE_FIXED: return _tr("Fixed");
        case CURVE_MIN_MAX:  return _tr("Min/Max");
        case CURVE_ZERO_MAX: return _tr("Zero/Max");
        case CURVE_GT_ZERO:  return "> 0"; //Don't translate these
        case CURVE_LT_ZERO:  return "< 0"; //Don't translate these
        case CURVE_ABSVAL:   return _tr("ABSVAL");
        case CURVE_EXPO:     return _tr("EXPO");
        case CURVE_DEADBAND: return _tr("Deadband");
        case CURVE_3POINT:   return _tr("3 Point");
        case CURVE_5POINT:   return _tr("5 Point");
        case CURVE_7POINT:   return _tr("7 Point");
        case CURVE_9POINT:   return _tr("9 Point");
        case CURVE_11POINT:  return _tr("11 Point");
        case CURVE_13POINT:  return _tr("13 Point");
    }
    return _tr("Unknown");
}

u8 CURVE_NumPoints(struct Curve *curve)
{
    switch (CURVE_TYPE(curve)) {
        case CURVE_NONE:
        case CURVE_FIXED:
        case CURVE_MIN_MAX:
        case CURVE_ZERO_MAX:
        case CURVE_GT_ZERO:
        case CURVE_LT_ZERO:
        case CURVE_ABSVAL:
            return 0;
        case CURVE_EXPO:
        case CURVE_DEADBAND:
             return 2;
        default:
             return (CURVE_TYPE(curve) + 1 - CURVE_3POINT) * 2 + 1;
    }
}

