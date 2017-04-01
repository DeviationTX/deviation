#ifndef _MIXER_H_
#define _MIXER_H_

#define DEFAULT_SERVO_LIMIT 150
#define SWASH_INV_ELEVATOR_MASK   1
#define SWASH_INV_AILERON_MASK    2
#define SWASH_INV_COLLECTIVE_MASK 4

//MAX = 10000
//MIN = -10000
#define CHAN_MULTIPLIER 100
#define PCT_TO_RANGE(x) ((x) * CHAN_MULTIPLIER)
#define RANGE_TO_PCT(x) ((x) / CHAN_MULTIPLIER)
#define CHAN_MAX_VALUE (100 * CHAN_MULTIPLIER)
#define CHAN_MIN_VALUE (-100 * CHAN_MULTIPLIER)
#define NUM_CHANNELS (NUM_OUT_CHANNELS + NUM_VIRT_CHANNELS)
#define NUM_SOURCES (NUM_INPUTS + NUM_CHANNELS + MAX_PPM_IN_CHANNELS)

#define CURVE_TYPE(x)       (((x)->type) & 0x7F)
#define CURVE_SMOOTHING(x)  (((x)->type) & 0x80)
#define CURVE_SET_TYPE(x, y) ((x)->type = ((x)->type & ~0x7F) | (y))
#define CURVE_SET_SMOOTHING(x, y) ((x)->type = ((x)->type & ~0x80) | ((y) ? 0x80 : 0))

#define MIXER_MUX(x)        (((x)->flags) & 0x0F)
#define MIXER_APPLY_TRIM(x) (((x)->flags) & 0x10)
#define MIXER_SET_APPLY_TRIM(x,y) ((x)->flags = ((x)->flags & ~0x10) | ((y) ? 0x10 : 0))
#define MIXER_SET_MUX(x,y)        ((x)->flags = ((x)->flags & ~0x0F) | (y))

enum {
    TRIM_ONOFF     = 191,
    TRIM_TOGGLE    = 192,
    TRIM_MOMENTARY = 193,
    TRIM_SWITCH_TYPES = 3,
};

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

enum PPMInMode {
    PPM_IN_OFF,
    PPM_IN_TRAIN1,
    PPM_IN_TRAIN2,
    PPM_IN_SOURCE,
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
    //u8 index;
    //s8 p1;
    //s8 p2;
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
    MUX_DELAY,
#if HAS_EXTENDED_AUDIO
    MUX_BEEP,
    MUX_VOICE,
#endif
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
    struct Curve curve;
    u8 src;
    u8 dest;
    u8 sw;
    s8 scalar;
    s8 offset;
    u8 flags;
    //apply_trim;
    //enum MuxType mux;
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
    s16 safetyval;  // allow safetyval to be over +/-125
    u8 max;
    u8 min;
    u8 servoscale;
    u8 servoscale_neg;
    s8 failsafe;
    u8 speed;     //measured in degrees/100msec
    s16 subtrim;  // need to support value greater than 250
};

struct Trim {
    u8 src;
    u8 pos;
    u8 neg;
    u8 step;
    u8 sw;
    s8 value[3];
};

/* Curve functions */
s32 CURVE_Evaluate(s32 value, struct Curve *curve);
const char *CURVE_GetName(char *str, struct Curve *curve);
unsigned CURVE_NumPoints(struct Curve *curve);

/* Mixer functions */
volatile s32 *MIXER_GetInputs();
s32 MIXER_GetChannel(unsigned channel, enum LimitMask flags);

int MIXER_GetMixers(int ch, struct Mixer *mixers, int count);
int MIXER_SetMixers(struct Mixer *mixers, int count);

struct Limit *MIXER_GetLimit(int ch);
void MIXER_SetLimit(int ch, struct Limit *limit);

void MIXER_SetTemplate(int ch, enum TemplateType value);
enum TemplateType MIXER_GetTemplate(int ch);

void MIXER_InitMixer(struct Mixer *mixer, unsigned ch);

void MIXER_ApplyMixer(struct Mixer *mixer, volatile s32 *raw, s32 *orig_value);
void MIXER_EvalMixers(volatile s32 *raw);
int MIXER_GetCachedInputs(s32 *raw, unsigned threshold);

struct Mixer *MIXER_GetAllMixers();

struct Trim *MIXER_GetAllTrims();

void MIXER_RegisterTrimButtons();

s32 MIXER_ApplyLimits(unsigned channel, struct Limit *limit, volatile s32 *_raw,
                      volatile s32 *__Channels, enum LimitMask flags);
void MIXER_SetDefaultLimit(struct Limit *limit);
const char *MIXER_TemplateName(enum TemplateType t);
const char *MIXER_SwashType(enum SwashType);
unsigned MIXER_SourceHasTrim(unsigned src);
unsigned MIXER_MapChannel(unsigned channel);
unsigned MIXER_UpdateTrim(u32 buttons, unsigned flags, void *data);
s8 *MIXER_GetTrim(unsigned i);
s32 MIXER_GetTrimValue(int i);
int MIXER_GetSourceVal(int idx, u32 opts);
int MIXER_SourceAsBoolean(unsigned src);

void MIXER_Init();

#endif
