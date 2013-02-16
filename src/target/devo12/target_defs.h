#ifndef _DEVO12_TARGET_H_
#define _DEVO12_TARGET_H_

#define VECTOR_TABLE_LOCATION 0x4000
#define SPIFLASH_SECTOR_OFFSET 0
#define SPIFLASH_SECTORS 512

#define TRANSPARENT_COLOR 0x10000

//#define ENABLE_SCANNER 1
#define HAS_TOUCH 1
#define HAS_VIBRATINGMOTOR 0
#define HAS_LOGICALVIEW 0
#define MIN_BRIGHTNESS 1 
#define DEFAULT_BATTERY_ALARM 4000
#define DEFAULT_BATTERY_CRITICAL 3800
#define MAX_BATTERY_ALARM 12000
#define MIN_BATTERY_ALARM 3300

//Protocols
#define PROTO_HAS_CYRF6936
#define PROTO_HAS_A7105

#define NUM_OUT_CHANNELS 12
#define NUM_VIRT_CHANNELS 10

#define NUM_TRIMS 6
#define MAX_POINTS 13
#define NUM_MIXERS ((NUM_OUT_CHANNELS + NUM_VIRT_CHANNELS) * 4)

#define INP_HAS_CALIBRATION 10

/* Compute voltage from y = 0.001662x - 0.03988 */
#define VOLTAGE_NUMERATOR 166
#define VOLTAGE_OFFSET     20

#include "devo12_ports.h"

//Use 2nd ROM for media
#define MEDIA_DRIVE PARFlash_ReadBytes, PARFlash_ReadBytesStopCR, NULL, NULL, 0
int PARFlash_ReadBytesStopCR(u32 readAddress, u32 length, u8 * buffer);
void PARFlash_ReadBytes(u32 readAddress, u32 length, u8 * buffer);

#endif //_DEVO12_TARGET_H_
