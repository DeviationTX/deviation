#ifndef _MIXER_H_
#define _MIXER_H_

#define CURVE_TYPE(x) x.num_points
#define NO_CURVE   0x00
#define MIN_MAX    0x80
#define ZERO_MAX   0x81
#define GT_ZERO    0x82
#define LT_ZERO    0x83
#define ABSVAL     0x84

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

int MIX_GetMixers(int ch, struct Mixer *mixers, int count);
void MIX_SetTemplate(int ch, int value);
int MIX_GetTemplate(int ch);

#endif
