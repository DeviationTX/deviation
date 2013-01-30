#ifndef _DEVO_H_
#define _DEVO_H_

#include "ports.h"

void ADC_Init(void);
void ADC_StartCapture();

enum {
    TIMER_SOUND = LAST_PRIORITY,
    NUM_MSEC_CALLBACKS,
};
void CLOCK_ClearMsecCallback(int MsecCallback);
u32 SOUND_Callback();

extern void PROTO_Stubs(int);
// ADC defines
#define NUM_ADC_CHANNELS (INP_HAS_CALIBRATION + 1) //Inputs + Voltage
extern const u8 adc_chan_sel[NUM_ADC_CHANNELS];
extern u16 adc_array_raw[NUM_ADC_CHANNELS];

#endif
