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
s16 expo(struct Curve *curve, s16 value)
{

    s16  y;
    s16 k;
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

s16 CURVE_Evaluate(s16 xval, struct Curve *curve)
{
    switch (curve->type) {
        case CURVE_NONE:     return xval;
        case CURVE_FIXED:    return CHAN_MAX_VALUE;
        case CURVE_MIN_MAX:  return (xval < 0) ? CHAN_MIN_VALUE : CHAN_MAX_VALUE;
        case CURVE_ZERO_MAX: return (xval < 0) ? 0 : CHAN_MAX_VALUE;
        case CURVE_GT_ZERO:  return (xval < 0) ? 0 : xval;
        case CURVE_LT_ZERO:  return (xval > 0) ? 0 : xval;
        case CURVE_ABSVAL:   return (xval < 0) ? -xval : xval;
        case CURVE_EXPO:     return expo(curve, xval);
        default:             return interpolate(curve, xval);
    }
}

const char *CURVE_GetName(struct Curve *curve)
{
    switch (curve->type) {
        case CURVE_NONE: return "1-to-1";
        case CURVE_FIXED: return "Fixed";
        case CURVE_MIN_MAX:  return "Min/Max";
        case CURVE_ZERO_MAX: return "Zero/Max";
        case CURVE_GT_ZERO:  return "> 0";
        case CURVE_LT_ZERO:  return "< 0";
        case CURVE_ABSVAL:   return "ABSVAL";
        case CURVE_EXPO:     return "EXPO";
        case CURVE_3POINT:   return "3 Point";
        case CURVE_5POINT:   return "5 Point";
        case CURVE_7POINT:   return "7 Point";
        case CURVE_9POINT:   return "9 Point";
        case CURVE_11POINT:  return "11 Point";
        case CURVE_13POINT:  return "13 Point";
    }
    return "Unknown";
}

u8 CURVE_NumPoints(struct Curve *curve)
{
    switch (curve->type) {
        case CURVE_NONE:
        case CURVE_MIN_MAX:
        case CURVE_ZERO_MAX:
        case CURVE_GT_ZERO:
        case CURVE_LT_ZERO:
        case CURVE_ABSVAL:
            return 0;
        case CURVE_EXPO:
             return 2;
        default:
             return (curve->type - CURVE_EXPO) * 2 + 1;
    }
}

