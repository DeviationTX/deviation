#ifndef _CRSF_H_
#define _CRSF_H_

#include <telemetry.h>

#define UART_SYNC       0xC8

// Device addresses
#define ADDR_BROADCAST  0x00  //  Broadcast address
#define ADDR_USB        0x10  //  USB Device
#define ADDR_BLUETOOTH  0x12  //  Bluetooth Module
// Custom Telemetry Frames 0x7F,0x80
#define CRSF_FRAMETYPE_AP_CUSTOM_TELEM_LEGACY  0x7F     // as suggested by Remo Masina for fw < 4.06 (Ardupilot)
#define CRSF_FRAMETYPE_AP_CUSTOM_TELEM         0x80     // reserved for ArduPilot by TBS, requires fw >= 4.06 (conflict with next?)
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
#define ADDR_ELRS_LUA   0xEF  //  ELRS

// Frame Type
#define TYPE_GPS              0x02
#define TYPE_VARIO            0x07
#define TYPE_BATTERY          0x08
#define TYPE_BARO_ALT         0x09
#define TYPE_HEARTBEAT        0x0b
#define TYPE_VTX              0x0F
#define TYPE_VTX_TELEM        0x10
#define TYPE_LINK             0x14
#define TYPE_CHANNELS         0x16
#define TYPE_RX_ID            0x1C
#define TYPE_TX_ID            0x1D
#define TYPE_ATTITUDE         0x1E
#define TYPE_FLIGHT_MODE      0x21
#define TYPE_PING_DEVICES     0x28
#define TYPE_DEVICE_INFO      0x29
#define TYPE_REQUEST_SETTINGS 0x2A
#define TYPE_SETTINGS_ENTRY   0x2B
#define TYPE_SETTINGS_READ    0x2C
#define TYPE_SETTINGS_WRITE   0x2D
#define TYPE_ELRS_INFO        0x2E
#define TYPE_COMMAND_ID       0x32
#define TYPE_RADIO_ID         0x3A

// Frame Subtype
#define ACK_SUBCMD                     0xFF
// Command ID Subcommands
#define GENERAL_SUBCMD                 0x0A
#define   SUBCMD_SPD_PROPOSAL            0x70
#define   SUBCMD_SPD_RESPONSE            0x71
// Radio ID Subcommands
#define CRSF_SUBCOMMAND                0x10
#define   COMMAND_MODEL_SELECT_ID        0x05

#define TELEMETRY_RX_PACKET_SIZE   64
#define CRSF_MAX_FIXEDID          63

enum {
    PROTO_OPTS_BITRATE,
    PROTO_OPTS_HIDDEN,
    PROTO_OPTS_ELRSARM,
    LAST_PROTO_OPT,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);

#if SUPPORT_CRSF_CONFIG

#define CRSF_MAX_DEVICES       4
#define CRSF_MAX_NAME_LEN      20
#define CRSF_MAX_PARAMS        100     // one extra required, max observed is 50 in XF Transmitter
#define CRSF_MAX_STRING_BYTES  5000    // max observed is 2010 in Nano RX, TBS WiFi depends on number of networks
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

typedef enum {
// Higher values have higher priority
    FLAG_NONE = 0x00,    // No flags
    FLAG_CONN = 0x01,    // Connected
    FLAG_MMIS = 0x04,    // Model Mismatch
    FLAG_ARMD = 0x08,    // Armed
    FLAG_UNU0 = 0x20,    // Not while connected (not used)
    FLAG_UNU1 = 0x40,    // Baud rate too low (not used)
} elrs_info_flags_t; 

typedef struct {
    u8 update;
    u8 bad_pkts;
    u16 good_pkts;
    elrs_info_flags_t flags;
    char flag_info[CRSF_MAX_NAME_LEN];
} elrs_info_t;

typedef enum {
    MODULE_UNKNOWN,
    MODULE_ELRS,
    MODULE_OTHER,
} module_type_t;

typedef struct {
    // common fields
    u8 device;            // device index of device parameter belongs to
    u8 id;                // Parameter number (starting from 1)
    u8 parent;            // Parent folder parameter number of the parent folder, 0 means root
    enum data_type type;  // (Parameter type definitions and hidden bit)
    volatile u8 hidden:1; // set if hidden
    volatile u8 loaded:1; // clear to force reload
    u8 lines_per_row:2;   // GUI optimization
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
    int parent_row_idx;   // GUI optimization
    int child_row_idx;    // GUI optimization
} crsf_param_t;

extern crsf_device_t crsf_devices[CRSF_MAX_DEVICES];
extern elrs_info_t elrs_info;

void CRSF_serial_rcv(u8 *buffer, u8 num_bytes);
u8 CRSF_serial_txd(u8 *buffer);
u8 crsf_crc8(const u8 *ptr, u8 len);
u8 crsf_crc8_BA(const u8 *ptr, u8 len);
void crsf_crc8_acc(u8 *crc, const u8 val);
void crsf_crc8_BA_acc(u8 *crc, const u8 val);
void CRSF_ping_devices(u8 address);
void CRSF_read_param(u8 device, u8 id, u8 chunk);
void CRSF_set_param(crsf_param_t *param);
void CRSF_send_command(crsf_param_t *param, enum cmd_status status);
u8 CRSF_send_model_id(u8 fixed_id);
u32 CRSF_read_timeout();
void CRSF_get_elrs();
void protocol_read_params(u8 device_idx, crsf_param_t param[]);
void protocol_set_param(crsf_param_t *param);
void protocol_module_info(module_type_t type, u32 firmware_id);
u8 protocol_module_is_elrs(u8 maj_version);
u8 protocol_elrs_is_armed();
u8 CRSF_speed_response(u8 accept, usart_callback_t tx_callback);
u8 CRSF_speed_proposal(u32 bitrate);
u8 CRSF_command_ack(u8 cmd_id, u8 sub_cmd_id, u8 accept);

#endif  // SUPPORT_CRSF_CONFIG

#endif
