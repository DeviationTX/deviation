#ifndef _DEVO12_TARGET_H_
#define _DEVO12_TARGET_H_

#include "../common/devo/common_devo.h"

#define TXID 0x0c
#define VECTOR_TABLE_LOCATION 0x4000
#define SPIFLASH_SECTOR_OFFSET 0
#define SPIFLASH_SECTORS 512

#define TRANSPARENT_COLOR 0x10000

#define HAS_STANDARD_GUI    1
#define HAS_ADVANCED_GUI    1
#define HAS_PERMANENT_TIMER 1
#define HAS_TELEMETRY       1
#define HAS_TOUCH           1
#define HAS_RTC             1
#define HAS_VIBRATINGMOTOR  0
#define HAS_DATALOG         1
#define HAS_SCANNER         0
#define ENABLE_320x240_GUI  1 //Enable support for 320x240 gui items as well as 480x360 ones

#define MIN_BRIGHTNESS 1 
#define DEFAULT_BATTERY_ALARM 4000
#define DEFAULT_BATTERY_CRITICAL 3500
#define MAX_BATTERY_ALARM 12000
#define MIN_BATTERY_ALARM 3300
#define MAX_POWER_ALARM 60

#define NUM_OUT_CHANNELS 12
#define NUM_VIRT_CHANNELS 10

#define NUM_TRIMS 10
#define NUM_TOGGLES 4
#define MAX_POINTS 13
#define NUM_MIXERS ((NUM_OUT_CHANNELS + NUM_VIRT_CHANNELS) * 4)

#define INP_HAS_CALIBRATION 10

#define NUM_ELEMS 50
/* Compute voltage from y = 0.001662x - 0.03988 */
#define VOLTAGE_NUMERATOR 160
#define VOLTAGE_OFFSET    268

/* Define button placement for chantest page*/
/* these are supposed to more closely match real Tx button layout */
/* negative value: box at the right side of the label */
#define CHANTEST_BUTTON_PLACEMENT { \
    {51, 120}, {51, 105}, {-229, 120}, {-229, 105}, \
    {-106, 135}, {21, 135}, {-259, 135}, {174, 135}, \
    {30, 71}, {30, 56}, {-250, 71}, {-250, 56}, \
    {185, 220}, {185, 200}, {-95, 220}, {-95, 200}, {200, 180}, {-80, 180}, \
    }

#include "devo12_ports.h"

//Use 2nd ROM for media
#define MEDIA_DRIVE PARFlash_ReadBytes, PARFlash_ReadBytesStopCR, NULL, NULL, 0
int PARFlash_ReadBytesStopCR(u32 readAddress, u32 length, u8 * buffer);
void PARFlash_ReadBytes(u32 readAddress, u32 length, u8 * buffer);

#endif //_DEVO12_TARGET_H_
