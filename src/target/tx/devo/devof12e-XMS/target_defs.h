#ifndef _DEVO_F12E_XMS_TARGET_H_
#define _DEVO_F12E_XMS_TARGET_H_

#define TXID 0xEC
#define VECTOR_TABLE_LOCATION 0x4000
#define HAS_FLASH_DETECT 1
#define SPIFLASH_SECTOR_OFFSET 0

#define LCD_WIDTH 66
#define LCD_HEIGHT 26
#define CHAR_WIDTH 12
#define CHAR_HEIGHT 18

#define HAS_STANDARD_GUI    1
#define HAS_ADVANCED_GUI    1
#define HAS_PERMANENT_TIMER 1
#define HAS_TELEMETRY       1
#define HAS_EXTENDED_TELEMETRY 1
#define HAS_TOUCH           0
#define HAS_RTC             0
#define HAS_VIBRATINGMOTOR  1
#define HAS_DATALOG         1
#define SUPPORT_SCANNER         0
#define HAS_LAYOUT_EDITOR   0
#define HAS_EXTRA_SWITCHES  0
#define HAS_EXTRA_BUTTONS   0
#define HAS_MULTIMOD_SUPPORT 1
#define HAS_MAPPED_GFX      1
#define USE_PBM_IMAGE       1
#define HAS_CHAR_ICONS      1
#define HAS_VIDEO           32
#define HAS_EXTENDED_AUDIO  1
#define HAS_AUDIO_UART     0
#define HAS_MUSIC_CONFIG    1

#ifdef BUILDTYPE_DEV
   #define DEBUG_WINDOW_SIZE 200
#else
   #define DEBUG_WINDOW_SIZE 0
#endif

#define MIN_BRIGHTNESS 0
#define DEFAULT_BATTERY_ALARM 9600
#define DEFAULT_BATTERY_CRITICAL 3900
#define MAX_BATTERY_ALARM 12000
#define MIN_BATTERY_ALARM 3300
#define MAX_POWER_ALARM 60

#define NUM_OUT_CHANNELS 16
#define NUM_VIRT_CHANNELS 10

#define NUM_TRIMS 10
#define MAX_POINTS 13
#define NUM_MIXERS ((NUM_OUT_CHANNELS + NUM_VIRT_CHANNELS) * 4)

#define INP_HAS_CALIBRATION 8

/* Compute voltage from y = 3.261x + 507 */
#define VOLTAGE_NUMERATOR 326
#define VOLTAGE_OFFSET    507

#include "../devof12e/hardware.h"
#include "../common/common_devo.h"

#endif //_DEVO_F12E_XMS_TARGET_H_
