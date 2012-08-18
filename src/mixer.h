#ifndef _MIXER_H_
#define _MIXER_H_

//MAX = 10000
//MIN = -10000
#define CHAN_MULTIPLIER 100
#define PCT_TO_RANGE(x) ((s16)(x) * CHAN_MULTIPLIER)
#define RANGE_TO_PCT(x) ((s16)(x) / CHAN_MULTIPLIER)
#define CHAN_MAX_VALUE (100 * CHAN_MULTIPLIER)
#define CHAN_MIN_VALUE (-100 * CHAN_MULTIPLIER)

enum Safety {
    SAFE_NONE,
    SAFE_MIN,
    SAFE_ZERO,
    SAFE_MAX,
};

enum Mode {
    MODE_1=1,
    MODE_2=2,
    MODE_3=3,
    MODE_4=4
};

enum CurveType {
    CURVE_NONE,
    CURVE_FIXED,
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
extern const char *tx_stick_names[4];
extern const char *tx_button_str[NUM_TX_BUTTONS];

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
#define MIXER_SRC(x) ((x) & 0x7F)
#define MIXER_SRC_IS_INV(x) ((x) & 0x80)
#define MIXER_SET_SRC_INV(x, y) x = (y) ? ((x) | 0x80) : ((x) & ~0x80)
struct Mixer {
    u8 src;
    u8 dest;
    u8 sw;
    struct Curve curve;
    s8 scalar;
    s8 offset;
    enum MuxType mux;
};

enum LimitFlags {
    CH_REVERSE     = 0x01,
    CH_FAILSAFE_EN = 0x02,
};

enum LimitMask {
    APPLY_REVERSE = 0x01,
    APPLY_LIMITS  = 0x02,
    APPLY_SAFETY  = 0x04,
    APPLY_SUBTRIM = 0x08,
    APPLY_ALL     = 0xFF,
};

struct Limit {
    u8 flags;
    u8 safetysw;
    s8 safetyval;
    s8 max;
    s8 min;
    s8 failsafe;
    s8 subtrim;
};

struct Trim {
    u8 src;
    u8 pos;
    u8 neg;
    u8 step;
    s16 value;
};

/* Curve functions */
s16 CURVE_Evaluate(s16 value, struct Curve *curve);
const char *CURVE_GetName(struct Curve *curve);
u8 CURVE_NumPoints(struct Curve *curve);

/* Mixer functions */
s16 *MIXER_GetInputs();
s16 MIXER_GetChannel(u8 channel, enum LimitMask flags);

int MIXER_GetMixers(int ch, struct Mixer *mixers, int count);
int MIXER_SetMixers(struct Mixer *mixers, int count);

void MIXER_GetLimit(int ch, struct Limit *limit);
void MIXER_SetLimit(int ch, struct Limit *limit);

void MIXER_SetTemplate(int ch, enum TemplateType value);
enum TemplateType MIXER_GetTemplate(int ch);

void MIXER_InitMixer(struct Mixer *mixer, u8 ch);

void MIXER_ApplyMixer(struct Mixer *mixer, s16 *raw);
void MIXER_EvalMixers(s16 *raw);
u8   MIXER_ReadInputs(s16 *raw, u8 threshold);
void MIXER_CreateCyclicInputs(s16 *raw);

struct Mixer *MIXER_GetAllMixers();

struct Trim *MIXER_GetAllTrims();

void MIXER_RegisterTrimButtons();

s16 MIXER_ApplyLimits(u8 channel, struct Limit *limit, s16 *raw, enum LimitMask flags);
const char *MIXER_SourceName(char *str, u8 src);
const char *MIXER_TemplateName(enum TemplateType t);
const char *MIXER_ButtonName(u8 src);
const char *MIXER_SwashType(enum SwashType);
u8 MIXER_MapChannel(u8 channel);
void MIXER_AdjustForProtocol();

void MIXER_Init();

#endif
