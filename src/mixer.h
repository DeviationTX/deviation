#ifndef _MIXER_H_
#define _MIXER_H_

enum CurveType {
    CURVE_NONE,
    CURVE_MIN_MAX,
    CURVE_ZERO_MAX,
    CURVE_GT_ZERO,
    CURVE_LT_ZERO,
    CURVE_ABSVAL,
    CURVE_EXPO,
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

//The followingis defined bythe target
extern const char *tx_input_str[NUM_TX_INPUTS];

enum TemplateType {
    MIXERTEMPLATE_NONE,
    MIXERTEMPLATE_SIMPLE,
    MIXERTEMPLATE_EXPO_DR,
    MIXERTEMPLATE_COMPLEX,
};
#define MIXERTEMPLATE_MAX MIXERTEMPLATE_COMPLEX

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
#define MIX_SRC(x) ((x) & 0x7F)
#define MIX_SRC_IS_INV(x) ((x) & 0x80)
#define MIX_SET_SRC_INV(x, y) x = (y) ? ((x) | 0x80) : ((x) & ~0x80)
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
    u8 reverse;
    u8 safetysw;
    s8 safetyval;
    s8 max;
    s8 min;
};

/* Curve functions */
s16 CURVE_Evaluate(s16 value, struct Curve *curve);
const char *CURVE_GetName(struct Curve *curve);

/* Mixer functions */
int MIX_GetMixers(int ch, struct Mixer *mixers, int count);
int MIX_SetMixers(struct Mixer *mixers, int count);

void MIX_GetLimit(int ch, struct Limit *limit);
void MIX_SetLimit(int ch, struct Limit *limit);

void MIX_SetTemplate(int ch, enum TemplateType value);
enum TemplateType MIX_GetTemplate(int ch);

void MIX_InitMixer(struct Mixer *mixer, u8 ch);

void MIX_ApplyMixer(struct Mixer *mixer, s16 *raw);
void MIX_EvalMixers(s16 *raw);
u8 MIX_ReadInputs(s16 *raw);
void MIX_CreateCyclicInputs(s16 *raw);
struct Mixer *MIX_GetAllMixers();

s16 MIX_ApplyLimits(u8 channel, struct Limit *limit, s16 *raw);
#endif
