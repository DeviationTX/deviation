#ifndef _DEVO8_TARGET_H_
#define _DEVO8_TARGET_H_

//Protocols
#define PROTO_HAS_A7105
#define PROTO_HAS_CYRF6936

#define MIN_BRIGHTNESS 0

#define NUM_OUT_CHANNELS 12
#define NUM_VIRT_CHANNELS 10

#define NUM_TRIMS 6
#define MAX_POINTS 13
#define NUM_MIXERS ((NUM_OUT_CHANNELS + NUM_VIRT_CHANNELS) * 4)

#define INP_HAS_CALIBRATION 4

#if 0
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
};
#endif
#define CHAN_ButtonMask(btn) (btn ? (1 << (btn - 1)) : 0)

#endif //_DEVO8_TARGET_H_
