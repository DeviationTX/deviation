#ifndef _TELEMETRY_H_
#define _TELEMETRY_H_

#define NUM_TELEM 10
struct Telemetry {
    u16 volt[3];
    u8 temp[4];
    u16 rpm[3];
    u8 line1[12];
    u8 line2[12];
};
extern struct Telemetry Telemetry; 
s32 TELEMETRY_GetValue(int idx);
const char * TELEMETRY_SetString(char *str, u8 telem);
const char * TELEMETRY_Name(char *str, u8 telem);
#endif
