#ifndef _DEVO7E_TARGET_H_
#define _DEVO7E_TARGET_H_

#include "../common/devo/common_devo.h"

#define TXID 0x7e
#define VECTOR_TABLE_LOCATION 0x3000
#define SPIFLASH_SECTOR_OFFSET 0
#define SPIFLASH_SECTORS 512

#if defined BUILDTYPE_DEV && ! defined EMULATOR
//No room for debug and standard gui
 #define HAS_STANDARD_GUI   0
#else
 #define HAS_STANDARD_GUI   1
#endif

#define HAS_ADVANCED_GUI    1
#define HAS_PERMANENT_TIMER 0
#define HAS_TELEMETRY       1
#define HAS_TOUCH           0
#define HAS_RTC             0
#define HAS_VIBRATINGMOTOR  1
#define HAS_DATALOG         0
#define HAS_SCANNER         0
#define HAS_LAYOUT_EDITOR   0
#define HAS_EXTRA_SWITCHES  OPTIONAL
#define HAS_MULTIMOD_SUPPORT 1
#define HAS_VIDEO           0

#define NO_LANGUAGE_SUPPORT 1

#define DEBUG_WINDOW_SIZE 0
#define MIN_BRIGHTNESS 0
#define DEFAULT_BATTERY_ALARM 4100
#define DEFAULT_BATTERY_CRITICAL 3900
#define MAX_BATTERY_ALARM 9000
#define MIN_BATTERY_ALARM 3300
#define MAX_POWER_ALARM 60

#define NUM_OUT_CHANNELS 12
#define NUM_VIRT_CHANNELS 10

#define NUM_TRIMS 6
#define NUM_TOGGLES 5
#define MAX_POINTS 13
#define NUM_MIXERS ((NUM_OUT_CHANNELS + NUM_VIRT_CHANNELS) * 4)

#define INP_HAS_CALIBRATION 4

/* Compute voltage from y = 2.1592x + 0.2493 */
#define VOLTAGE_NUMERATOR 216
#define VOLTAGE_OFFSET    249

#endif //_DEVO7E_TARGET_H_

