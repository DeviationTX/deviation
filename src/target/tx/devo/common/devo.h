#ifndef _DEVO_H_
#define _DEVO_H_

#ifndef HAS_STANDARD_GUI
    #define HAS_STANDARD_GUI 1
#endif
#ifndef HAS_ADVANCED_GUI
    #define HAS_ADVANCED_GUI 1
#endif

void ADC_Init(void);
void ADC_StartCapture();

void CLOCK_ClearMsecCallback(int MsecCallback);
u32 SOUND_Callback();

extern void PROTO_Stubs(int);
// ADC defines
#define NUM_ADC_CHANNELS (INP_HAS_CALIBRATION + 2) //Inputs + Temprature + Voltage
extern volatile u16 adc_array_raw[NUM_ADC_CHANNELS];
void ADC_Filter();

#endif
