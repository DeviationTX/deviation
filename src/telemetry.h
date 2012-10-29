#ifndef _TELEMETRY_H_
#define _TELEMETRY_H_

#define NUM_TELEM 9
#define TELEM_ERROR_TIME 5000

enum {
    TELEM_VOLT1,
    TELEM_VOLT2,
    TELEM_VOLT3,
    TELEM_TEMP1,
    TELEM_TEMP2,
    TELEM_TEMP3,
    TELEM_TEMP4,
    TELEM_RPM1,
    TELEM_RPM2,
    TELEM_GPS_LAT,
    TELEM_GPS_LONG,
    TELEM_GPS_ALT,
    TELEM_GPS_SPEED,
    TELEM_GPS_TIME,
};

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
    u32 time[3];
};
extern struct Telemetry Telemetry; 
s32 TELEMETRY_GetValue(int idx);
const char * TELEMETRY_GetValueStr(char *str, u8 telem);
const char * TELEMETRY_Name(char *str, u8 telem);
#endif
