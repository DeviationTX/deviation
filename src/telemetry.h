#ifndef _TELEMETRY_H_
#define _TELEMETRY_H_

#if !defined(HAS_EXTENDED_TELEMETRY)
#define HAS_EXTENDED_TELEMETRY 0
#endif

#define TELEM_ERROR_TIME 5000
#define TELEM_NUM_ALARMS 6


enum {
    TELEM_FRSKY,
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
#if HAS_EXTENDED_TELEMETRY
    TELEM_DSM_PBOX_VOLT1,
    TELEM_DSM_PBOX_VOLT2,
    TELEM_DSM_PBOX_CAPACITY1,
    TELEM_DSM_PBOX_CAPACITY2,
    TELEM_DSM_PBOX_ALARMV1,
    TELEM_DSM_PBOX_ALARMV2,
    TELEM_DSM_PBOX_ALARMC1,
    TELEM_DSM_PBOX_ALARMC2,
#endif
    TELEM_DSM_AIRSPEED,
    TELEM_DSM_ALTITUDE,
    TELEM_DSM_ALTITUDE_MAX,
    TELEM_DSM_GFORCE_X,
    TELEM_DSM_GFORCE_Y,
    TELEM_DSM_GFORCE_Z,
    TELEM_DSM_GFORCE_XMAX,
    TELEM_DSM_GFORCE_YMAX,
    TELEM_DSM_GFORCE_ZMAX,
    TELEM_DSM_GFORCE_ZMIN,
#if HAS_EXTENDED_TELEMETRY
    TELEM_DSM_JETCAT_STATUS,
    TELEM_DSM_JETCAT_THROTTLE,
    TELEM_DSM_JETCAT_PACKVOLT,
    TELEM_DSM_JETCAT_PUMPVOLT,
    TELEM_DSM_JETCAT_RPM,
    TELEM_DSM_JETCAT_TEMPEGT,
    TELEM_DSM_JETCAT_OFFCOND,
    TELEM_DSM_RXPCAP_AMPS,
    TELEM_DSM_RXPCAP_CAPACITY,
    TELEM_DSM_RXPCAP_VOLT,
    TELEM_DSM_FPCAP_AMPS,
    TELEM_DSM_FPCAP_CAPACITY,
    TELEM_DSM_FPCAP_TEMP,
#endif
    TELEM_DSM_VARIO_ALTITUDE,
    TELEM_DSM_VARIO_CLIMBRATE1,
    TELEM_DSM_VARIO_CLIMBRATE2,
    TELEM_DSM_VARIO_CLIMBRATE3,
    TELEM_DSM_VARIO_CLIMBRATE4,
    TELEM_DSM_VARIO_CLIMBRATE5,
    TELEM_DSM_VARIO_CLIMBRATE6,
#if HAS_EXTENDED_TELEMETRY
    TELEM_DSM_ESC_AMPS1,
    TELEM_DSM_ESC_AMPS2,
    TELEM_DSM_ESC_VOLT1,
    TELEM_DSM_ESC_VOLT2,
    TELEM_DSM_ESC_TEMP1,
    TELEM_DSM_ESC_TEMP2,
    TELEM_DSM_ESC_RPM,
    TELEM_DSM_ESC_THROTTLE,
    TELEM_DSM_ESC_OUTPUT,
#endif
    TELEM_DSM_LAST,
};

#ifdef HAS_EXTENDED_AUDIO
enum {
    TELEM_UNIT_NONE = 0,
    TELEM_UNIT_TEMP,
    TELEM_UNIT_VOLT,
    TELEM_UNIT_RPM,
    TELEM_UNIT_AMPS,
    TELEM_UNIT_ALTITUDE,
    TELEM_UNIT_GFORCE,
};
#endif

// FrSky telemetry stream state machine
typedef enum {
  TS_IDLE = 0,  // waiting for 0x5e frame marker
  TS_DATA_ID,   // waiting for dataID
  TS_DATA_LOW,  // waiting for data low byte
  TS_DATA_HIGH, // waiting for data high byte
  TS_DATA_END,  // waiting for 0x5e end of frame marker
  TS_XOR = 0x80 // decode stuffed byte
} TS_STATE;


typedef enum {
    TELEM_FRSKY_RSSI = 1,
    TELEM_FRSKY_VOLT1,
    TELEM_FRSKY_VOLT2,
    TELEM_FRSKY_LQI,
    TELEM_FRSKY_LRSSI,
#if HAS_EXTENDED_TELEMETRY
    TELEM_FRSKY_VOLT3,
    TELEM_FRSKY_VOLTA,
    TELEM_FRSKY_TEMP1,
    TELEM_FRSKY_TEMP2,
    TELEM_FRSKY_RPM,
    TELEM_FRSKY_MIN_CELL,
    TELEM_FRSKY_ALL_CELL,
    TELEM_FRSKY_CELL1,
    TELEM_FRSKY_CELL2,
    TELEM_FRSKY_CELL3,
    TELEM_FRSKY_CELL4,
    TELEM_FRSKY_CELL5,
    TELEM_FRSKY_CELL6,
    TELEM_FRSKY_FUEL,
    TELEM_FRSKY_CURRENT,
    TELEM_FRSKY_ALTITUDE,
    TELEM_FRSKY_VARIO,
    TELEM_FRSKY_DISCHARGE,    // mAh
#endif
    TELEM_FRSKY_LAST
} frsky_telem_t;

#define TELEM_VALS        (((int)TELEM_DSM_LAST > (int)TELEM_DEVO_LAST)            \
                               ? (((int)TELEM_DSM_LAST > (int)TELEM_FRSKY_LAST)    \
                                   ? (int)TELEM_DSM_LAST : (int)TELEM_FRSKY_LAST)  \
                               : (((int)TELEM_DEVO_LAST > (int)TELEM_FRSKY_LAST)   \
                                   ? (int)TELEM_DEVO_LAST : (int)TELEM_FRSKY_LAST) \
                          )
#define NUM_TELEM   TELEM_VALS - 1
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
    TELEMFLAG_ALARM1 = 0x01,
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
    u32 velocity;
    u32 time;
    u16 heading;
    u8 satcount;
};

#define TELEM_UPDATE_SIZE (((TELEM_VALS + 7) + 31) / 32)
struct Telemetry {
    struct gps gps;
    s32 value[TELEM_VALS];
    u16 capabilities;
    volatile u32 updated[TELEM_UPDATE_SIZE];
};

enum {
    PROTO_TELEM_UNSUPPORTED = -1,
    PROTO_TELEM_OFF = 0,
    PROTO_TELEM_ON  = 1,
};


/************************************************************************/
/*  SPort telemetry                                                     */
/************************************************************************/
// FrSky old DATA IDs (1 byte)
#define GPS_ALT_BP_ID             0x01
#define TEMP1_ID                  0x02
#define RPM_ID                    0x03
#define FUEL_ID                   0x04
#define TEMP2_ID                  0x05
#define VOLTS_ID                  0x06
#define GPS_ALT_AP_ID             0x09
#define BARO_ALT_BP_ID            0x10
#define GPS_SPEED_BP_ID           0x11
#define GPS_LONG_BP_ID            0x12
#define GPS_LAT_BP_ID             0x13
#define GPS_COURS_BP_ID           0x14
#define GPS_DAY_MONTH_ID          0x15
#define GPS_YEAR_ID               0x16
#define GPS_HOUR_MIN_ID           0x17
#define GPS_SEC_ID                0x18
#define GPS_SPEED_AP_ID           0x19
#define GPS_LONG_AP_ID            0x1A
#define GPS_LAT_AP_ID             0x1B
#define GPS_COURS_AP_ID           0x1C
#define BARO_ALT_AP_ID            0x21
#define GPS_LONG_EW_ID            0x22
#define GPS_LAT_NS_ID             0x23
#define ACCEL_X_ID                0x24
#define ACCEL_Y_ID                0x25
#define ACCEL_Z_ID                0x26
#define CURRENT_ID                0x28
#define VARIO_ID                  0x30
#define VFAS_ID                   0x39
#define VOLTS_BP_ID               0x3A
#define VOLTS_AP_ID               0x3B
#define FRSKY_LAST_ID             0x3F
#define D_RSSI_ID                 0xF0
#define D_A1_ID                   0xF1
#define D_A2_ID                   0xF2

#define VFAS_D_HIPREC_OFFSET      2000

// FrSky new DATA IDs (2 bytes)
#define ALT_FIRST_ID              0x0100
#define ALT_LAST_ID               0x010f
#define VARIO_FIRST_ID            0x0110
#define VARIO_LAST_ID             0x011f
#define CURR_FIRST_ID             0x0200
#define CURR_LAST_ID              0x020f
#define VFAS_FIRST_ID             0x0210
#define VFAS_LAST_ID              0x021f
#define CELLS_FIRST_ID            0x0300
#define CELLS_LAST_ID             0x030f
#define T1_FIRST_ID               0x0400
#define T1_LAST_ID                0x040f
#define T2_FIRST_ID               0x0410
#define T2_LAST_ID                0x041f
#define RPM_FIRST_ID              0x0500
#define RPM_LAST_ID               0x050f
#define FUEL_FIRST_ID             0x0600
#define FUEL_LAST_ID              0x060f
#define ACCX_FIRST_ID             0x0700
#define ACCX_LAST_ID              0x070f
#define ACCY_FIRST_ID             0x0710
#define ACCY_LAST_ID              0x071f
#define ACCZ_FIRST_ID             0x0720
#define ACCZ_LAST_ID              0x072f
#define GPS_LONG_LATI_FIRST_ID    0x0800
#define GPS_LONG_LATI_LAST_ID     0x080f
#define GPS_ALT_FIRST_ID          0x0820
#define GPS_ALT_LAST_ID           0x082f
#define GPS_SPEED_FIRST_ID        0x0830
#define GPS_SPEED_LAST_ID         0x083f
#define GPS_COURS_FIRST_ID        0x0840
#define GPS_COURS_LAST_ID         0x084f
#define GPS_TIME_DATE_FIRST_ID    0x0850
#define GPS_TIME_DATE_LAST_ID     0x085f
#define A3_FIRST_ID               0x0900
#define A3_LAST_ID                0x090f
#define A4_FIRST_ID               0x0910
#define A4_LAST_ID                0x091f
#define AIR_SPEED_FIRST_ID        0x0a00
#define AIR_SPEED_LAST_ID         0x0a0f
#define POWERBOX_BATT1_FIRST_ID   0x0b00
#define POWERBOX_BATT1_LAST_ID    0x0b0f
#define POWERBOX_BATT2_FIRST_ID   0x0b10
#define POWERBOX_BATT2_LAST_ID    0x0b1f
#define POWERBOX_STATE_FIRST_ID   0x0b20
#define POWERBOX_STATE_LAST_ID    0x0b2f
#define POWERBOX_CNSP_FIRST_ID    0x0b30
#define POWERBOX_CNSP_LAST_ID     0x0b3f
#define RSSI_ID                   0xf101
#define ADC1_ID                   0xf102
#define ADC2_ID                   0xf103
#define SP2UART_A_ID              0xfd00
#define SP2UART_B_ID              0xfd01
#define BATT_ID                   0xf104
#define SWR_ID                    0xf105
#define XJT_VERSION_ID            0xf106

/************************************************************************/

extern struct Telemetry Telemetry; 
s32 TELEMETRY_GetValue(int idx);
s32 _TELEMETRY_GetValue(struct Telemetry *t, int idx);
const char * TELEMETRY_GetValueStr(char *str, int idx);
const char * TELEMETRY_GetValueStrByValue(char *str, int idx, s32 value);
const char * TELEMETRY_Name(char *str, int idx);
const char * TELEMETRY_ShortName(char *str, int idx);
s32 TELEMETRY_GetMaxValue(int idx);
s32 TELEMETRY_GetMinValue(int idx);
void TELEMETRY_Alarm();
void TELEMETRY_ResetAlarm(int i);
void TELEMETRY_MuteAlarm();
int TELEMETRY_HasAlarm(int src);
u32 TELEMETRY_IsUpdated(int val);
void TELEMETRY_SetUpdated(int telem);
int TELEMETRY_Type();
void TELEMETRY_SetType(int type);
int TELEMETRY_GetNumTelemSrc();
#endif
