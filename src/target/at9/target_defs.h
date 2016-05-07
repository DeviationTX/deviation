#ifndef _AT9_TARGET_DEFS_H_
#define _AT9_TARGET_DEFS_H_

#define TXID 0xA9
#define VECTOR_TABLE_LOCATION 0x3000

#define SPIFLASH_SECTOR_OFFSET 0x0000
#define SPIFLASH_SECTORS 64

#define USE_PBM_IMAGE 1
#define USE_DEVOFS 1 //Must be before common_devo include

#define DISABLE_PWM 1                 //FIXME
#define NO_LANGUAGE_SUPPORT 1
#include "devofs.h"
#define FILE_SIZE sizeof(FATFS)

    #define fs_mount      df_mount
    #define fs_open       df_open
    #define fs_read       df_read
    #define fs_lseek      df_lseek
    #define fs_opendir    df_opendir
    #define fs_readdir    df_readdir
    #define fs_write      df_write
    #define fs_switchfile df_switchfile
    #define fs_maximize_file_size() if(0) {}
    #define fs_set_drive_num(x,num) if(0) {}
    #define fs_get_drive_num(x) 0
    #define fs_is_open(x) ((x)->file_cur_pos != -1)
    #define fs_close(x) df_close()
    #define fs_filesize(x) (((x)->file_header.size1 << 8) | (x)->file_header.size2)
    #define fs_ltell(x)   ((x)->file_cur_pos)

static inline void LCD_ForceUpdate() {}

#define USE_4BUTTON_MODE    1
#define HAS_STANDARD_GUI    0
#define HAS_ADVANCED_GUI    1
#define HAS_PERMANENT_TIMER 1
#define HAS_TELEMETRY       1
#define HAS_TOUCH           0
#define HAS_RTC             0
#define HAS_VIBRATINGMOTOR  1
#define HAS_DATALOG         1
#define HAS_LAYOUT_EDITOR   1
#define HAS_SCANNER         0
#define HAS_EXTRA_SWITCHES  0
#define HAS_EXTRA_BUTTONS  0
#define HAS_MULTIMOD_SUPPORT 0
#define HAS_VIDEO           0

#ifdef BUILDTYPE_DEV
   #define DEBUG_WINDOW_SIZE 200
#else
   #define DEBUG_WINDOW_SIZE 0
#endif

#define MIN_BRIGHTNESS 0
#define DEFAULT_BATTERY_ALARM 8000
#define DEFAULT_BATTERY_CRITICAL 7500
#define MAX_BATTERY_ALARM 12000
#define MIN_BATTERY_ALARM 5500
#define MAX_POWER_ALARM 60

#define NUM_OUT_CHANNELS 12
#define NUM_VIRT_CHANNELS 10

#define NUM_TRIMS 10
#define NUM_TOGGLES 5
#define MAX_POINTS 13
#define NUM_MIXERS ((NUM_OUT_CHANNELS + NUM_VIRT_CHANNELS) * 4)

#define INP_HAS_CALIBRATION 8

/* Compute voltage from y = 0.003672x + 0.0287 */
#define VOLTAGE_NUMERATOR 367
#define VOLTAGE_OFFSET    29
#include "at9_ports.h"


/* Define button placement for chantest page*/
/* these are supposed to more closely match real Tx button layout */
/* negative value: box at the right side of the label */
#define CHANTEST_BUTTON_PLACEMENT { \
    {51, 120}, {51, 105}, {-229, 120}, {-229, 105}, \
    {-106, 135}, {21, 135}, {-259, 135}, {174, 135}, \
    {30, 71}, {30, 56}, {-250, 71}, {-250, 56}, \
    {185, 220}, {185, 200}, {-95, 220}, {-95, 200}, {200, 180}, {-80, 180}, \
    }
#endif //_AT9_TARGET_DEFS_H_
