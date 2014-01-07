#ifndef _TELEMETRY_H_
#define _TELEMETRY_H_

#define NUM_TELEM 9
#define TELEM_ERROR_TIME 5000
#define TELEM_NUM_ALARMS 6

enum {
    TELEM_DEVO,
    TELEM_DSM,
};

enum {
    TELEM_DEVO_VOLT1 = 1,
    TELEM_DEVO_VOLT2,
    TELEM_DEVO_VOLT3,
    TELEM_DEVO_TEMP1,
    TELEM_DEVO_TEMP2,
    TELEM_DEVO_TEMP3,
    TELEM_DEVO_TEMP4,
    TELEM_DEVO_RPM1,
    TELEM_DEVO_RPM2,
    TELEM_DEVO_LAST,
};
enum {
    TELEM_DSM_FLOG_FADESA = 1,
    TELEM_DSM_FLOG_FADESB,
    TELEM_DSM_FLOG_FADESL,
    TELEM_DSM_FLOG_FADESR,
    TELEM_DSM_FLOG_FRAMELOSS,
    TELEM_DSM_FLOG_HOLDS,
    TELEM_DSM_FLOG_VOLT1,
    TELEM_DSM_FLOG_VOLT2,
    TELEM_DSM_FLOG_RPM1,
    TELEM_DSM_FLOG_TEMP1,
    TELEM_DSM_AMPS1,
    TELEM_DSM_PBOX_VOLT1,
    TELEM_DSM_PBOX_VOLT2,
    TELEM_DSM_PBOX_CAPACITY1,
    TELEM_DSM_PBOX_CAPACITY2,
    TELEM_DSM_PBOX_ALARMV1,
    TELEM_DSM_PBOX_ALARMV2,
    TELEM_DSM_PBOX_ALARMC1,
    TELEM_DSM_PBOX_ALARMC2,
    TELEM_DSM_AIRSPEED,
    TELEM_DSM_ALTITUDE,
    TELEM_DSM_GFORCE_X,
    TELEM_DSM_GFORCE_Y,
    TELEM_DSM_GFORCE_Z,
    TELEM_DSM_GFORCE_XMAX,
    TELEM_DSM_GFORCE_YMAX,
    TELEM_DSM_GFORCE_ZMAX,
    TELEM_DSM_GFORCE_ZMIN,
    TELEM_DSM_JETCAT_STATUS,
    TELEM_DSM_JETCAT_THROTTLE,
    TELEM_DSM_JETCAT_PACKVOLT,
    TELEM_DSM_JETCAT_PUMPVOLT,
    TELEM_DSM_JETCAT_RPM,
    TELEM_DSM_JETCAT_TEMPEGT,
    TELEM_DSM_JETCAT_OFFCOND,
    TELEM_DSM_LAST,
};
#define TELEM_VALS        (((int)TELEM_DSM_LAST > (int)TELEM_DEVO_LAST) ? (int)TELEM_DSM_LAST : (int)TELEM_DEVO_LAST)
enum {
    TELEM_GPS_LAT = TELEM_VALS,
    TELEM_GPS_LONG,
    TELEM_GPS_ALT,
    TELEM_GPS_SPEED,
    TELEM_GPS_TIME,
    TELEM_GPS_SATCOUNT,
    TELEM_GPS_HEADING,
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
    u32 heading;
    u32 satcount;
};

struct telem_devo {
    u16 volt[3];
    s16 temp[4];
    u16 rpm[2];
};
struct telem_dsm_flog {
    u8 fades[4];
    u8 frameloss;
    u8 holds;
    u16 volt[2];
    u16 rpm;
    s16 temp;
};

struct telem_dsm_pbox {
    u16 volt[2];
    u16 capacity[2];
    u16 alarmv[2];
    u16 alarmc[2];
};
struct telem_dsm_sensors {
    u16 amps;
    u16 airspeed;
    u16 altitude;
};
struct telem_dsm_gforce {
    u16 x;
    u16 y;
    u16 z;
    u16 xmax;
    u16 ymax;
    u16 zmax;
    u16 zmin;
};

struct telem_dsm_jetcat {
    u8 status;
    u8 offcond;
    u16 throttle;
    u16 packvolt;
    u16 pumpvolt;
    u16 rpm;
    u16 temp_egt;
};

struct telem_dsm {
    struct telem_dsm_flog    flog;
    struct telem_dsm_pbox    pbox;
    struct telem_dsm_sensors sensors;
    struct telem_dsm_gforce  gforce;
    struct telem_dsm_jetcat  jetcat;
};

#define TELEM_UPDATE_SIZE (((TELEM_VALS + 7)+ 7) / 8)
struct Telemetry {
    union {
        struct telem_devo devo;
        struct telem_dsm  dsm;
    } p;
    struct gps gps;
    u16 capabilities;
    volatile u8 updated[TELEM_UPDATE_SIZE];
};
extern struct Telemetry Telemetry; 
s32 TELEMETRY_GetValue(int idx);
s32 _TELEMETRY_GetValue(struct Telemetry *t, int idx);
const char * TELEMETRY_GetValueStr(char *str, unsigned telem);
const char * TELEMETRY_GetValueStrByValue(char *str, unsigned telem, s32 value);
const char * TELEMETRY_Name(char *str, unsigned telem);
const char * TELEMETRY_ShortName(char *str, unsigned telem);
s32 TELEMETRY_GetMaxValue(unsigned telem);
void TELEMETRY_Alarm();
int TELEMETRY_HasAlarm(int src);
u32 TELEMETRY_IsUpdated(int val);
void TELEMETRY_SetUpdated(int telem);
int TELEMETRY_Type();
void TELEMETRY_SetType(int type);
void TELEMETRY_SetTypeByProtocol(enum Protocols protocol);
#endif
