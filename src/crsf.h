#ifndef _CRSF_H_
#define _CRSF_H_

#include <telemetry.h>

// Device addresses
#define ADDR_BROADCAST  0x00  //  Broadcast address
#define ADDR_USB        0x10  //  USB Device
#define ADDR_BLUETOOTH  0x12  //  Bluetooth Module
#define ADDR_PRO_CORE   0x80  //  TBS CORE PNP PRO
//  #define ADDR_  0x8A       //  Reserved
#define ADDR_PRO_CURR   0xC0  //  PNP PRO digital current sensor
#define ADDR_PRO_GPS    0xC2  //  PNP PRO GPS
#define ADDR_BLACKBOX   0xC4  //  TBS Blackbox
#define ADDR_FC         0xC8  //  Flight controller
//  #define ADDR_       0xCA  //  Reserved
#define ADDR_RACETAG    0xCC  //  Race tag
#define ADDR_RADIO      0xEA  //  Radio Transmitter
//  #define ADDR_       0xEB  //  Reserved
#define ADDR_RECEIVER   0xEC  //  Crossfire / UHF receiver
#define ADDR_MODULE     0xEE  //  Crossfire transmitter

// Frame Type
#define TYPE_GPS              0x02
#define TYPE_BATTERY          0x08
#define TYPE_VIDEO            0x08
#define TYPE_LINK             0x14
#define TYPE_CHANNELS         0x16
#define TYPE_ATTITUDE         0x1E
#define TYPE_FLIGHT_MODE      0x21
#define TYPE_PING_DEVICES     0x28
#define TYPE_DEVICE_INFO      0x29
#define TYPE_REQUEST_SETTINGS 0x2A
#define TYPE_SETTINGS_ENTRY   0x2B
#define TYPE_SETTINGS_READ    0x2C
#define TYPE_SETTINGS_WRITE   0x2D

#define TELEMETRY_RX_PACKET_SIZE   64

#if SUPPORT_CRSF_CONFIG

#define CRSF_MAX_DEVICES       4
#define CRSF_MAX_NAME_LEN      16
#define CRSF_MAX_STRING_BYTES  2500     // max observed is 2010 in Nano RX
#define CRSF_STRING_BYTES_AVAIL(current)  (CRSF_MAX_STRING_BYTES-((char *)(current)-mp->strings))


enum data_type {
    UINT8          = 0,
    INT8           = 1,
    UINT16         = 2,
    INT16          = 3,
    FLOAT          = 8,
    TEXT_SELECTION = 9,
    STRING         = 10,
    FOLDER         = 11,
    INFO           = 12,
    COMMAND        = 13,
    OUT_OF_RANGE   = 127,
};

enum cmd_status {
    READY               = 0,
    START               = 1,
    PROGRESS            = 2,
    CONFIRMATION_NEEDED = 3,
    CONFIRM             = 4,
    CANCEL              = 5,
    POLL                = 6
};

typedef struct {
    u8 address;
    u8 number_of_params;
    u8 params_version;
    u32 serial_number;
    u32 hardware_id;
    u32 firmware_id;
    char name[CRSF_MAX_NAME_LEN];
} crsf_device_t;

typedef struct {
    // common fields
    u8 device;            // device index of device parameter belongs to
    u8 id;                // Parameter number (starting from 1)
    u8 parent;            // Parent folder parameter number of the parent folder, 0 means root
    enum data_type type;  // (Parameter type definitions and hidden bit)
    u8 hidden;            // set if hidden
    char *name;           // Null-terminated string
    void *value;          // size depending on data type

    // field presence depends on type
    void *default_value;  // size depending on data type. Not present for COMMAND.
    s32 min_value;        // not sent for string type
    s32 max_value;        // not sent for string type
    s32 step;             // Step size ( type float only otherwise this entry is not sent )
    u8 timeout;           // COMMAND timeout (100ms/count)
    u8 changed;           // flag if set needed when edit element is de-selected
    char *max_str;        // Longest choice length for text select
    union {
        u8 point;             // Decimal point ( type float only otherwise this entry is not sent )
        u8 text_sel;          // current value index for TEXT_SELECTION type
        u8 string_max_len;    // String max length ( for string type only )
        u8 status;            // Status for COMMANDs
    } u;
    union {
        char *info;
        char *unit;         // Unit ( Null-terminated string / not sent for type string and folder )
    } s;
} crsf_param_t;

extern crsf_device_t crsf_devices[CRSF_MAX_DEVICES];

void CRSF_serial_rcv(u8 *buffer, u8 num_bytes);
u8 CRSF_serial_txd(u8 *buffer, u8 max_len);
u8 crsf_crc8(const u8 *ptr, u8 len);
void CRSF_ping_devices();
void CRSF_read_param(u8 device, u8 id, u8 chunk);
void CRSF_set_param(crsf_param_t *param);
void CRSF_send_command(crsf_param_t *param, enum cmd_status status);

#endif  // SUPPORT_CRSF_CONFIG

#endif
