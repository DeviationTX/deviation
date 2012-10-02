#ifndef _TELEMETRY_H_
#define _TELEMETRY_H_

enum {
    Box_None,
    Box_Timer1,
    Box_Timer2,
    Box_Telemetry1,
    Box_Telemetry2,
    Box_LAST,
};

struct Telemetry {
    u16 volt1;
    u16 volt2;
    u16 volt3;
};
extern struct Telemetry Telemetry; 
s32 TELEMETRY_GetValue(int idx);
void TELEMETRY_SetString(char *str, s32 value);
#endif
