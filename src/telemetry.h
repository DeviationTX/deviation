#ifndef _TELEMETRY_H_
#define _TELEMETRY_H_

#define NUM_TELEM 9
#define TELEM_ERROR_TIME 5000
#define TELEM_NUM_ALARMS 6

enum {
    TELEM_VOLT1 = 1,
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
enum {
    TELEMFLAG_ALARM1 = 0x00,
    TELEMFLAG_ALARM2 = 0x02,
    TELEMFLAG_ALARM3 = 0x04,
    TELEMFLAG_ALARM4 = 0x08,
    TELEMFLAG_ALARM5 = 0x10,
    TELEMFLAG_ALARM6 = 0x20,
};

enum {
    TELEMUNIT_FEET   = 0x40,
    TELEMUNIT_FAREN  = 0x80,
};
struct gps {
    s32 latitude;
    s32 longitude;
    s32 altitude;
    s32 velocity;
    u32 time;
};

struct Telemetry {
    u16 volt[3];
    s16 temp[4];
    u16 rpm[2];
    struct gps gps;
    u32 time[3];
};
extern struct Telemetry Telemetry; 
s32 TELEMETRY_GetValue(int idx);
const char * TELEMETRY_GetValueStr(char *str, u8 telem);
const char * TELEMETRY_GetValueStrByValue(char *str, u8 telem, s32 value);
const char * TELEMETRY_Name(char *str, u8 telem);
const char * TELEMETRY_ShortName(char *str, u8 telem);
s32 TELEMETRY_GetMaxValue(u8 telem);
void TELEMETRY_Alarm();
int TELEMETRY_HasAlarm(int src);
#endif
