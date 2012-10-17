#ifndef _TELEMETRY_H_
#define _TELEMETRY_H_

#define NUM_TELEM 10
struct gps {
    s32 latitude;
    s32 longitude;
    s32 altitude;
    s32 velocity;
    u8  hour;
    u8  min;
    u8  sec;
    u8  day;
    u8  month;
    u16 year;
};

struct Telemetry {
    u16 volt[3];
    u8 temp[4];
    u16 rpm[2];
    struct gps gps;
};
extern struct Telemetry Telemetry; 
s32 TELEMETRY_GetValue(int idx);
const char * TELEMETRY_SetString(char *str, u8 telem);
const char * TELEMETRY_GetGPS(char *str, u8 line);
const char * TELEMETRY_Name(char *str, u8 telem);
#endif
