#ifndef _AT9_TARGET_DEFS_H_
#define _AT9_TARGET_DEFS_H_

#define TXID 0xA9
#define VECTOR_TABLE_LOCATION 0x3000

#define SPIFLASH_SECTOR_OFFSET 0x0000
#if defined HAS_4IN1_FLASH && HAS_4IN1_FLASH
#define SPIFLASH_SECTORS 1024

// Various SPI flash memories use different commands to
// block-protect memory and to write more than 1 byte
// Possible variants:
// ISSI IS25CQ032
// Microchip SST25VF032B - original Devo 10
// Microchip SST26VF032B - not fully supported, block protection needs work
// adesto AT25DF321A
// Winbond W25Q

// Microchip SST25VF032B and Winbond W25Q use EWSR (0x50) to enable write to status reg,
// everyone else uses WREN (0x06)
#define SPIFLASH_USE_EWSR 0

// Microchip SST25VF032B uses AAI (0xAD or 0xAF) to write more than 1 byte
// everyone else uses PAGE_PROG (0x02)
#define SPIFLASH_USE_AAI 0
//#define SPIFLASH_AAI_AF // SST25VF512A uses 0xAF for AAI

// Use STATUS register for global protect and use this constant
// 0x3C for AT25 and IS25
// 0x1C for W25Q and SST25B - correct value, not 0x38
// 0x0C for SST25A
// don't define for SST26 - it uses special block protection register
#define SPIFLASH_USE_GLOBAL_PROTECT 0x3C

#define SPIFLASH_USE_FAST_READ 1

#else

#define SPIFLASH_SECTORS 64

#define USE_PBM_IMAGE 1
#define USE_DEVOFS 1 //Must be before common_devo include

#endif

#define DISABLE_PWM 1                 //FIXME
#define NO_LANGUAGE_SUPPORT 1

#ifndef FATSTRUCT_SIZE
    #if defined USE_DEVOFS && USE_DEVOFS == 1
        #include "enable_devofs.h"
    #else
        #include "enable_petit_fat.h"
    #endif
#endif

#ifndef LCD_ForceUpdate
static inline void LCD_ForceUpdate() {}
#endif

#define USE_4BUTTON_MODE    1
#define HAS_STANDARD_GUI    0
#define HAS_ADVANCED_GUI    1
#define HAS_PERMANENT_TIMER 1
#define HAS_TELEMETRY       1
#define HAS_EXTENDED_TELEMETRY 1
#define HAS_TOUCH           0
#define HAS_RTC             0
#define HAS_VIBRATINGMOTOR  1
#define HAS_DATALOG         1
#define HAS_LAYOUT_EDITOR   1
#define HAS_SCANNER         0
#define HAS_EXTRA_SWITCHES  0
#define HAS_EXTRA_BUTTONS  0
#define HAS_MULTIMOD_SUPPORT 0
//#define HAS_4IN1_FLASH 1 // NB set in the Makefile.inc
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

#if defined HAS_4IN1_FLASH && HAS_4IN1_FLASH
void SPISwitch_Init();
void SPISwitch_CS_HI(int module);
void SPISwitch_CS_LO(int module);
void SPISwitch_UseFlashModule();
void SPISwitch_CYRF6936_RESET(int state);
void SPISwitch_NRF24L01_CE(int state);
#endif


#endif //_AT9_TARGET_DEFS_H_
