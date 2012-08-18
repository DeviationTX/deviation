#ifndef _DEVO8_TARGET_H_
#define _DEVO8_TARGET_H_

#define MIN_BRIGHTNESS 1 
//Protocols
#define PROTO_HAS_CYRF6936
#define PROTO_HAS_A7105

#define NUM_TX_BUTTONS 18
#define NUM_TX_INPUTS 14
#define NUM_INPUTS (NUM_TX_INPUTS + 3)
#define NUM_CHANNELS 12

#define NUM_TRIMS 6
#define MAX_POINTS 13
#define NUM_MIXERS (NUM_CHANNELS * 4)

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
    BUT_TRIM1_NEG,  /* LEFT-VERTICAL */
    BUT_TRIM1_POS,
    BUT_TRIM2_NEG,  /* RIGHT-VERTICAL */
    BUT_TRIM2_POS,
    BUT_TRIM3_NEG,  /* LEFT-HORIZONTAL */
    BUT_TRIM3_POS,
    BUT_TRIM4_NEG,  /* RIGHT-HORIZONTAL */
    BUT_TRIM4_POS,
    BUT_TRIM5_NEG,  /* LEFT */
    BUT_TRIM5_POS,
    BUT_TRIM6_NEG,  /* RIGHT */
    BUT_TRIM6_POS,
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

#define CHAN_ButtonMask(btn) (btn ? (1 << (btn - 1)) : 0)
#endif //_DEVO8_TARGET_H_
