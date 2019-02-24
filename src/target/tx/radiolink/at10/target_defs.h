#ifndef _AT10_TARGET_DEFS_H_
#define _AT10_TARGET_DEFS_H_

#define TXID 0xAA
#define VECTOR_TABLE_LOCATION 0xa000

#define SPIFLASH_SECTOR_OFFSET 0x0000

#if defined HAS_4IN1_FLASH && HAS_4IN1_FLASH
    #define SPIFLASH_SECTORS 1024
    #define SPIFLASH_TYPE IS25CQxxx
#else
    #define SPIFLASH_SECTORS 64
    #define USE_PBM_IMAGE 1
    #define USE_DEVOFS 1  // Must be before common_devo include
#endif

#define DISABLE_PWM 1                 // FIXME
#define SUPPORT_MULTI_LANGUAGE 0

#define HAS_LCD_TYPES       (LCDTYPE_ST7796)
#ifndef LCD_ForceUpdate
static inline void LCD_ForceUpdate() {}
#endif

#define HAS_STANDARD_GUI    1
#define HAS_ADVANCED_GUI    1
#define HAS_PERMANENT_TIMER 1
#define HAS_TELEMETRY       1
#define HAS_EXTENDED_TELEMETRY 1
#define HAS_TOUCH           0
#define HAS_RTC             0
#define HAS_VIBRATINGMOTOR  0
#define HAS_DATALOG         1
#define HAS_LAYOUT_EDITOR   1
#define HAS_SCANNER         0
#define HAS_EXTRA_SWITCHES  0
#define HAS_EXTRA_BUTTONS   0
#define HAS_MULTIMOD_SUPPORT 0
#define HAS_VIDEO           0
#define HAS_EXTENDED_AUDIO  0
#define HAS_AUDIO_UART      0
#define HAS_MUSIC_CONFIG    0
#define HAS_OLED_DISPLAY    0
#define ENABLE_320x240_GUI  1  // Enable support for 320x240 gui items as well as 480x320 ones

#ifdef BUILDTYPE_DEV
    #define DEBUG_WINDOW_SIZE 200
#else
    #define DEBUG_WINDOW_SIZE 0
#endif

#define LCD_WIDTH 480
#define LCD_HEIGHT 320

#define MIN_BRIGHTNESS 0
#define DEFAULT_BATTERY_ALARM 8000
#define DEFAULT_BATTERY_CRITICAL 7500
#define MAX_BATTERY_ALARM 12000
#define MIN_BATTERY_ALARM 5500
#define MAX_POWER_ALARM 60

#define NUM_OUT_CHANNELS 16
#define NUM_VIRT_CHANNELS 10

#define NUM_TRIMS 10
#define MAX_POINTS 13
#define NUM_MIXERS ((NUM_OUT_CHANNELS + NUM_VIRT_CHANNELS) * 4)

#define INP_HAS_CALIBRATION 9

/* Compute voltage from y = 0.003672x + 0.0287 */
#define VOLTAGE_NUMERATOR 367
#define VOLTAGE_OFFSET    29

/* Define button placement for chantest page*/
/* these are supposed to more closely match real Tx button layout */
/* negative value: box at the right side of the label */
#define CHANTEST_BUTTON_PLACEMENT { \
    {51, 120}, {51, 105}, {-229, 120}, {-229, 105}, \
    {-106, 135}, {21, 135}, {-259, 135}, {174, 135}, \
    {30, 71}, {30, 56}, {-250, 71}, {-250, 56}, \
    {185, 220}, {185, 200}, \
    }

#include "hardware.h"
#include "target/tx/radiolink/common/common_at.h"

#endif  // _AT10_TARGET_DEFS_H_
