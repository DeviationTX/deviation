#ifndef _DEVO_H_
#define _DEVO_H_

void ADC_Init(void);
u16 ADC1_Read(u8 channel);

enum {
    TIMER_SOUND = LAST_PRIORITY,
    NUM_MSEC_CALLBACKS,
};
void CLOCK_ClearMsecCallback(int MsecCallback);
u32 SOUND_Callback();
#endif
