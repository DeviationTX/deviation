#ifndef _T8SG_V2_TARGET_H_
#define _T8SG_V2_TARGET_H_

#define TXID 0x18
#define VECTOR_TABLE_LOCATION 0x3000
#define HAS_FLASH_DETECT 1
#define SPIFLASH_SECTOR_OFFSET 0

#define HAS_LCD_FLIPPED     1
#define LCD_CONTRAST_FUNC(x) (x)

#define HAS_STANDARD_GUI    1
#define HAS_ADVANCED_GUI    1
#define HAS_PERMANENT_TIMER 1
#define HAS_TELEMETRY       1
#define HAS_EXTENDED_TELEMETRY 1
#define HAS_TOUCH           0
#define HAS_RTC             0
#define HAS_VIBRATINGMOTOR  1
#define HAS_DATALOG         1
#define HAS_SCANNER         1
#define HAS_LAYOUT_EDITOR   1
#define HAS_EXTRA_SWITCHES  OPTIONAL
#define HAS_SWITCHES_NOSTOCK 1
#define HAS_EXTRA_BUTTONS   0
#define HAS_EXTRA_POTS      OPTIONAL
#define HAS_MULTIMOD_SUPPORT 1
#define HAS_VIDEO           0
#define HAS_4IN1_FLASH      0
#define HAS_EXTENDED_AUDIO  1
#define HAS_AUDIO_UART     1
#define HAS_MUSIC_CONFIG    1

#ifdef BUILDTYPE_DEV
  #define DEBUG_WINDOW_SIZE 200
#else
  #define DEBUG_WINDOW_SIZE 0
#endif

#define MIN_BRIGHTNESS 0
#define DEFAULT_BATTERY_ALARM 4100
#define DEFAULT_BATTERY_CRITICAL 3900
#define MAX_BATTERY_ALARM 12000
#define MIN_BATTERY_ALARM 3300
#define MAX_POWER_ALARM 60

#define NUM_OUT_CHANNELS 16
#define NUM_VIRT_CHANNELS 10

#define NUM_TRIMS 6
#define MAX_POINTS 13
#define NUM_MIXERS ((NUM_OUT_CHANNELS + NUM_VIRT_CHANNELS) * 4)

#if HAS_EXTRA_POTS
  #define INP_HAS_CALIBRATION 6
#else
  #define INP_HAS_CALIBRATION 4
#endif

/* Compute voltage from y = 2.1592x + 0.2493 */
#define VOLTAGE_NUMERATOR 216
#define VOLTAGE_OFFSET    249

#include "hardware.h"
#include "../common/common_devo.h"

#endif //_T8SG_V2_TARGET_H_
