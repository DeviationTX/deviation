#ifndef _DEVO10_TARGET_H_
#define _DEVO10_TARGET_H_

#include "fat_io.h"

#define TXID 0x9d
#define VECTOR_TABLE_LOCATION 0x0000
#define SPIFLASH_SECTOR_OFFSET 54     //FIXME
#define SPIFLASH_SECTORS 1024         //FIXME

#define DISABLE_PWM 1                 //FIXME

#define HAS_STANDARD_GUI    0         //FIXME
#define HAS_ADVANCED_GUI    1
#define HAS_PERMANENT_TIMER 1
#define HAS_TELEMETRY       1
#define HAS_EXTENDED_TELEMETRY 1
#define HAS_TOUCH           0
#define HAS_RTC             0         //FIXME
#define HAS_VIBRATINGMOTOR  0
#define HAS_DATALOG         1
#define HAS_SCANNER         0
#define HAS_LAYOUT_EDITOR   1
#define HAS_EXTRA_SWITCHES  0
#define HAS_EXTRA_BUTTONS  0
#define HAS_MULTIMOD_SUPPORT 1

#define MIN_BRIGHTNESS 0
#define DEFAULT_BATTERY_ALARM 6000
#define DEFAULT_BATTERY_CRITICAL 6500
#define MAX_BATTERY_ALARM 8000
#define MIN_BATTERY_ALARM 5500
#define MAX_POWER_ALARM 60

#define NUM_OUT_CHANNELS 16
#define NUM_VIRT_CHANNELS 10

#define NUM_TRIMS 8
#define MAX_POINTS 13
#define NUM_MIXERS ((NUM_OUT_CHANNELS + NUM_VIRT_CHANNELS) * 4)

#define INP_HAS_CALIBRATION 8

/* Compute voltage from y = 0.003246x + 0.4208 */
#define VOLTAGE_NUMERATOR 324
#define VOLTAGE_OFFSET    421

#include "x9d_ports.h"

#endif //_DEVO10_TARGET_H_
