#ifndef _COMMON_SIMPLE_H_
#define _COMMON_SIMPLE_H_

#include "mixer_simple.h"

typedef enum {
    PITTHROMODE_NORMAL = 0,
    PITTHROMODE_IDLE1,
    PITTHROMODE_IDLE2,
    PITTHROMODE_HOLD
} PitThroMode;

typedef enum {
    CURVESMODE_PITCH = 0,
    CURVESMODE_THROTTLE,
} CurvesMode;

const char *SIMPLEMIX_channelname_cb(guiObject_t *obj, const void *data);
void SIMPLEMIX_GetMixers(struct Mixer **mixers, u8 dest_channel, int count);
const char *SIMPLEMIX_ModeName(PitThroMode pit_mode);
s16 SIMPLEMIX_EvalMixerCb(s16 xval, struct Mixer *mix, s16 max_value, s16 min_value);

#endif
