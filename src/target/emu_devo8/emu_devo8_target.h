#ifndef _DEVO8_TARGET_H_
#define _DEVO8_TARGET_H_

#define NUM_TX_INPUTS 14 
#define NUM_INPUTS (NUM_TX_INPUTS + 3)
#define NUM_CHANNELS 12

//MAX = 10000
//MIN = -10000
#define CHAN_MULTIPLIER 100
#define PCT_TO_RANGE(x) ((s16)x * CHAN_MULTIPLIER)
#define RANGE_TO_PCT(x) ((s16)x / CHAN_MULTIPLIER)
#define CHAN_MAX_VALUE (100 * CHAN_MULTIPLIER)
#define CHAN_MIN_VALUE (-100 * CHAN_MULTIPLIER)

#define NUM_TRIMS 4
#define MAX_POINTS 13
#define NUM_MIXERS 16

#define INP_NONE     0
#define INP_THROTTLE 1
#define INP_RUDDER   2
#define INP_ELEVATOR 3
#define INP_AILERON  4
#define INP_RUD_DR   5
#define INP_ELE_DR   6
#define INP_AIL_DR   7 
#define INP_GEAR     8
#define INP_MIX0     9
#define INP_MIX1    10
#define INP_MIX2    11
#define INP_FMOD0   12
#define INP_FMOD1   13
#define INP_FMOD2   14

#define INP_HAS_CALIBRATION 4

#endif //_DEVO8_TARGET_H_
