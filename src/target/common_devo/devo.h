#ifndef _DEVO_H_
#define _DEVO_H_

void ADC_Init(void);
u16 ADC1_Read(u8 channel);

enum MsecCallback {
    TIMER_SOUND = 0,
};
#define NUM_MSEC_CALLBACKS 1

void CLOCK_SetMsecCallback(enum MsecCallback, u32 msec);
void CLOCK_ClearMsecCallback(enum MsecCallback);
u32 SOUND_Callback();
#endif
