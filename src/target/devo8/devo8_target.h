#ifndef _DEVO8_TARGET_H_
#define _DEVO8_TARGET_H_
 
//Protocols
#define PROTO_HAS_DEVO
#define PROTO_HAS_DSM2

#define NUM_TX_BUTTONS 14
#define NUM_TX_INPUTS 18
#define NUM_INPUTS (NUM_TX_INPUTS + 3)
#define NUM_CHANNELS 12

#define NUM_TRIMS 4
#define MAX_POINTS 13
#define NUM_MIXERS 16

#define INP_NONE     0
#define INP_AILERON  1
#define INP_ELEVATOR 2
#define INP_THROTTLE 3
#define INP_RUDDER   4
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

enum {
    BUT_NONE = 0,
    BUT_AILTRIM_NEG,
    BUT_AILTRIM_POS,
    BUT_ELETRIM_NEG,
    BUT_ELETRIM_POS,
    BUT_THRTRIM_NEG,
    BUT_THRTRIM_POS,
    BUT_RUDTRIM_NEG,
    BUT_RUDTRIM_POS,
    BUT_LFTTRIM_NEG,
    BUT_LFTTRIM_POS,
    BUT_RGTTRIM_NEG,
    BUT_RGTTRIM_POS,
    BUT_LEFT,
    BUT_RIGHT,
    BUT_DOWN,
    BUT_UP,
    BUT_ENTER,
    BUT_EXIT,
    BUT_NOCON_1,
    BUT_NOCON_2,
};
#define INP_HAS_CALIBRATION 4

#define CHAN_ButtonIsPressed(buttons, btn) ((btn) && ! ((buttons) & (1 << ((btn) - 1))))
#endif //_DEVO8_TARGET_H_
