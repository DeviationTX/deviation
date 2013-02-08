#ifndef _MIXER_SIMPLE_H_
#define _MIXER_SIMPLE_H_

typedef enum {
    GYROOUTPUT_GEAR = 4, // CH5
    GYROOUTPUT_AUX2 = 6, // CH7
} GyroOutputChannel;

typedef enum {
    SWITCHFUNC_FLYMODE = 0,
    SWITCHFUNC_HOLD,
    SWITCHFUNC_GYROSENSE,
    SWITCHFUNC_DREXP_AIL, // ALLOW dr/exp to use different switch than fly mode,however, they use the same switch by default ,
    SWITCHFUNC_DREXP_ELE, // ALLOW dr/exp to use different switch than fly mode,however, they use the same switch by default ,
    SWITCHFUNC_DREXP_RUD, // ALLOW dr/exp to use different switch than fly mode,however, they use the same switch by default ,
    SWITCHFUNC_LAST
} FunctionSwitch;

typedef struct {
    u8 aile;
    u8 elev;
    u8 throttle;
    u8 rudd;
    u8 pitch;
    u8 gear;
    u8 aux2;
    u8 actual_aile;
    u8 actual_elev;
    u8 switches[SWITCHFUNC_LAST];
} MappedSimpleChannels;

#define MAX_TRAVEL_LIMIT 175
#define PITCHMIXER_COUNT 4
#define THROTTLEMIXER_COUNT 3
#define DREXPMIXER_COUNT 3
#define GYROMIXER_COUNT 3

#define ALWAYSOFF_SWITCH (NUM_INPUTS + NUM_CHANNELS)  //virt10 as switch so that it won't be on
extern MappedSimpleChannels mapped_simple_channels;

#endif
