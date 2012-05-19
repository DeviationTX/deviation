#ifndef _MIXER_H_
#define _MIXER_H_

enum CurveType {
    CURVE_NONE,
    CURVE_MIN_MAX,
    CURVE_ZERO_MAX,
    CURVE_GT_ZERO,
    CURVE_LT_ZERO,
    CURVE_ABSVAL,
    CURVE_3POINT,
    CURVE_5POINT,
    CURVE_7POINT,
    CURVE_9POINT,
    CURVE_11POINT,
    CURVE_13POINT,
};
#define CURVE_MAX CURVE_13POINT


struct Curve {
    enum CurveType type;
    s8 points[MAX_POINTS];
};

enum TemplateType {
    MIXERTEMPLATE_NONE,
    MIXERTEMPLATE_SIMPLE,
    MIXERTEMPLATE_DR,
    MIXERTEMPLATE_COMPLEX,
    MIXERTEMPLATE_MAX,
};

enum MuxType {
    MUX_REPLACE,
    MUX_MULTIPLY,
    MUX_ADD,
};

enum SwashType {
    SWASH_TYPE_NONE,
    SWASH_TYPE_120,
    SWASH_TYPE_120X,
    SWASH_TYPE_140,
    SWASH_TYPE_90,
};
struct Mixer {
    u8 src;
    u8 dest;
    u8 sw;
    struct Curve curve;
    s8 scaler;
    s8 offset;
    enum MuxType mux;
};

struct Limit {
    s8 max;
    s8 min;
};

/* Curve functions */
s16 CURVE_Evaluate(s16 value, struct Curve *curve);
const char *CURVE_GetName(struct Curve *curve);

/* Mixer functions */
int MIX_GetMixers(int ch, struct Mixer *mixers, int count);
void MIX_GetLimit(int ch, struct Limit *limit);
void MIX_SetTemplate(int ch, int value);
int MIX_GetTemplate(int ch);

#endif
