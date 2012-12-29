#ifndef _MIXER_SIMPLE_H_
#define _MIXER_SIMPLE_H_

typedef enum {
    GYROOUTPUT_GEAR = 4, // CH5
    GYROOUTPUT_AUX2 = 6, // CH7
} GyroOutputChannel;

typedef enum {
    SWITCHFUNC_FLYMODE = 0,
    SWITCHFUNC_HOLD,
    SWITCHFUNC_GYROSENSE
} FunctionSwitch;

typedef struct {
    u8 aile;
    u8 elev;
    u8 throttle;
    u8 rudd;
    u8 pitch;
    u8 gear;
    u8 aux2;
    u8 swicthes[3];
} MappedSimpleChannels;

#define MAX_TRAVEL_LIMIT 150
#define PITCHMIXER_COUNT 4
#define THROTTLEMIXER_COUNT 3
#define DREXPMIXER_COUNT 3
#define GYROMIXER_COUNT 3

extern MappedSimpleChannels mapped_simple_channels;

#endif
