#ifndef _MIXER_H_
#define _MIXER_H_

#define DEFAULT_SERVO_LIMIT 150

//MAX = 10000
//MIN = -10000
#define CHAN_MULTIPLIER 100
#define PCT_TO_RANGE(x) ((s16)(x) * CHAN_MULTIPLIER)
#define RANGE_TO_PCT(x) ((s16)(x) / CHAN_MULTIPLIER)
#define CHAN_MAX_VALUE (100 * CHAN_MULTIPLIER)
#define CHAN_MIN_VALUE (-100 * CHAN_MULTIPLIER)
#define NUM_CHANNELS (NUM_OUT_CHANNELS + NUM_VIRT_CHANNELS)
#define NUM_SOURCES (NUM_INPUTS + NUM_CHANNELS)
enum Safety {
    SAFE_NONE,
    SAFE_MIN,
    SAFE_ZERO,
    SAFE_MAX,
    SAFE_COUNT,
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
    CURVE_DEADBAND,
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
extern const char *tx_stick_names[4];

enum TemplateType {
    MIXERTEMPLATE_NONE,
    MIXERTEMPLATE_SIMPLE,
    MIXERTEMPLATE_EXPO_DR,
    MIXERTEMPLATE_COMPLEX,
    MIXERTEMPLATE_CYC1,
    MIXERTEMPLATE_CYC2,
    MIXERTEMPLATE_CYC3,
};
#define MIXERTEMPLATE_MAX_HELI MIXERTEMPLATE_CYC3
#define MIXERTEMPLATE_MAX_PLANE MIXERTEMPLATE_COMPLEX
#define MIXERTEMPLATE_MAX (MIXERTEMPLATE_MAX_HELI > MIXERTEMPLATE_MAX_PLANE ? MIXERTEMPLATE_MAX_HELI : MIXERTEMPLATE_MAX_PLANE)

enum MuxType {
    MUX_REPLACE,
    MUX_MULTIPLY,
    MUX_ADD,
    MUX_MAX,
    MUX_MIN,
    MUX_LAST,
};

enum SwashType {
    SWASH_TYPE_NONE,
    SWASH_TYPE_120,
    SWASH_TYPE_120X,
    SWASH_TYPE_140,
    SWASH_TYPE_90,
    SWASH_TYPE_LAST,
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
    u8 apply_trim;
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
    APPLY_SCALAR  = 0x10,
    APPLY_SPEED   = 0x80,
    APPLY_ALL     = 0xFF,
};

struct Limit {
    u8 flags;
    u8 safetysw;
    s8 safetyval;
    u8 max;
    u8 min;
    u8 servoscale;
    s8 failsafe;
    u8 speed;     //measured in degrees/100msec
    s16 subtrim;  // need to support value greater than 250
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
volatile s16 *MIXER_GetInputs();
s16 MIXER_GetChannel(u8 channel, enum LimitMask flags);

int MIXER_GetMixers(int ch, struct Mixer *mixers, int count);
int MIXER_SetMixers(struct Mixer *mixers, int count);

void MIXER_GetLimit(int ch, struct Limit *limit);
void MIXER_SetLimit(int ch, struct Limit *limit);

void MIXER_SetTemplate(int ch, enum TemplateType value);
enum TemplateType MIXER_GetTemplate(int ch);

void MIXER_InitMixer(struct Mixer *mixer, u8 ch);

void MIXER_ApplyMixer(struct Mixer *mixer, volatile s16 *raw);
void MIXER_EvalMixers(volatile s16 *raw);
int MIXER_GetCachedInputs(s16 *raw, u8 threshold);

struct Mixer *MIXER_GetAllMixers();

struct Trim *MIXER_GetAllTrims();

void MIXER_RegisterTrimButtons();

s16 MIXER_ApplyLimits(u8 channel, struct Limit *limit, volatile s16 *_raw,
                      volatile s16 *_Channels, enum LimitMask flags);
void MIXER_SetDefaultLimit(struct Limit *limit);
const char *MIXER_TemplateName(enum TemplateType t);
const char *MIXER_SwashType(enum SwashType);
u8 MIXER_SourceHasTrim(u8 src);
u8 MIXER_MapChannel(u8 channel);
void MIXER_AdjustForProtocol();
u8 MIXER_UpdateTrim(u32 buttons, u8 flags, void *data);

void MIXER_Init();

#endif
