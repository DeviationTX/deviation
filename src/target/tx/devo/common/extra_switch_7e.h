#ifndef _EXTRA_SWITCH_7E_H_
#define _EXTRA_SWITCH_7E_H_

// Extra switch connections
//         2x2              3x1              3x2
//         -----            -----            -----
// B.5                                        SW_B0
// B.6      SW_B1            SW_A0            SW_B2
// B.7                                        SW_A0
// B.8      SW_A1            SW_A2            SW_A2
// global_extra_switches:
//   .0 == B0
//   .1 == B2
//   .2 == A0
//   .3 == A2

#define SWITCH_3x4  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2) \
                   | (1 << INP_SWC0) | (1 << INP_SWC1) | (1 << INP_SWC2) \
                   | (1 << INP_SWD0) | (1 << INP_SWD1) | (1 << INP_SWD2))
#define SWITCH_3x3  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2) \
                   | (1 << INP_SWC0) | (1 << INP_SWC1) | (1 << INP_SWC2))
#define SWITCH_3x2  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1) | (1 << INP_SWB2))
#define SWITCH_3x1  ((1 << INP_SWA0) | (1 << INP_SWA1) | (1 << INP_SWA2))
#define SWITCH_2x8  ((1 << INP_SWH0) | (1 << INP_SWH1) \
                   | (1 << INP_SWG0) | (1 << INP_SWG1) \
                   | (1 << INP_SWF0) | (1 << INP_SWF1) \
                   | (1 << INP_SWE0) | (1 << INP_SWE1) \
                   | (1 << INP_SWD0) | (1 << INP_SWD1) \
                   | (1 << INP_SWC0) | (1 << INP_SWC1) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1) \
                   | (1 << INP_SWA0) | (1 << INP_SWA1))
#define SWITCH_2x7  ((1 << INP_SWH0) | (1 << INP_SWH1) \
                   | (1 << INP_SWG0) | (1 << INP_SWG1) \
                   | (1 << INP_SWF0) | (1 << INP_SWF1) \
                   | (1 << INP_SWE0) | (1 << INP_SWE1) \
                   | (1 << INP_SWD0) | (1 << INP_SWD1) \
                   | (1 << INP_SWC0) | (1 << INP_SWC1) \
                   | (1 << INP_SWB0) | (1 << INP_SWB1))
#define SWITCH_2x6  ((1 << INP_SWH0) | (1 << INP_SWH1) \
                   | (1 << INP_SWG0) | (1 << INP_SWG1) \
                   | (1 << INP_SWF0) | (1 << INP_SWF1) \
                   | (1 << INP_SWE0) | (1 << INP_SWE1) \
                   | (1 << INP_SWD0) | (1 << INP_SWD1) \
                   | (1 << INP_SWC0) | (1 << INP_SWC1))
#define SWITCH_2x5  ((1 << INP_SWH0) | (1 << INP_SWH1) \
                   | (1 << INP_SWG0) | (1 << INP_SWG1) \
                   | (1 << INP_SWF0) | (1 << INP_SWF1) \
                   | (1 << INP_SWE0) | (1 << INP_SWE1) \
                   | (1 << INP_SWD0) | (1 << INP_SWD1))
#define SWITCH_2x4  ((1 << INP_SWH0) | (1 << INP_SWH1) \
                   | (1 << INP_SWG0) | (1 << INP_SWG1) \
                   | (1 << INP_SWF0) | (1 << INP_SWF1) \
                   | (1 << INP_SWE0) | (1 << INP_SWE1))
#define SWITCH_2x3  ((1 << INP_SWH0) | (1 << INP_SWH1) \
                   | (1 << INP_SWG0) | (1 << INP_SWG1) \
                   | (1 << INP_SWF0) | (1 << INP_SWF1))
#define SWITCH_2x2  ((1 << INP_SWH0) | (1 << INP_SWH1) \
                   | (1 << INP_SWG0) | (1 << INP_SWG1))
#define SWITCH_2x1  ((1 << INP_SWH0) | (1 << INP_SWH1))

#define SWITCH_STOCK ((1 << INP_HOLD0) | (1 << INP_HOLD1) \
                    | (1 << INP_FMOD0) | (1 << INP_FMOD1))

#define STOCK_INPUTS (SWITCH_STOCK \
                    | (1 << INP_AILERON) | (1 << INP_ELEVATOR) \
                    | (1 << INP_THROTTLE) | (1 << INP_RUDDER))

#define POT_2  ((1 << INP_AUX4) | (1 << INP_AUX5))
#define POT_1  (1 << INP_AUX4)

// Convert channel # to ignore_switch bit position
// Extra switches use upper 10 bits: CH(23) -> CH(32)
#define CH(x) (1 <<  ((x) - 1))

#define ADDON_SWITCH_CFG \
    ADDON_SWITCH("3x4", SWITCH_3x4, 0) \
    ADDON_SWITCH("3x3", SWITCH_3x3, 0) \
    ADDON_SWITCH("3x2", SWITCH_3x2, 0) \
    ADDON_SWITCH("3x1", SWITCH_3x1, 0) \
    ADDON_SWITCH("2x8", SWITCH_2x8, 0) \
    ADDON_SWITCH("2x7", SWITCH_2x7, 0) \
    ADDON_SWITCH("2x6", SWITCH_2x6, 0) \
    ADDON_SWITCH("2x5", SWITCH_2x5, 0) \
    ADDON_SWITCH("2x4", SWITCH_2x4, 0) \
    ADDON_SWITCH("2x3", SWITCH_2x3, 0) \
    ADDON_SWITCH("2x2", SWITCH_2x2, 0) \
    ADDON_SWITCH("2x1", SWITCH_2x1, 0) \
    ADDON_SWITCH("potx2", POT_2, 0) \
    ADDON_SWITCH("potx1", POT_1, 0) \
    ADDON_SWITCH("nostock", 0, SWITCH_STOCK)

#endif  // _EXTRA_SWITCH_7E_H_
