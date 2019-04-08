#ifndef _T12_TARGET_H_
#define _T12_TARGET_H_

#define FLASHTYPE FLASHTYPE_MMC
#if defined(EMULATOR) && EMULATOR == USE_NATIVE_FS
    #include "enable_native_fs.h"
#else
    #include "enable_fatfs.h"
    #include "target/drivers/storage/mmc_flash/fat_io.h"
    #include "target/drivers/serial/usb_cdc/usb_uart.h"
#endif

#define LCD_CONTRAST_FUNC(x) (x)

#define TXID 0x12
#define VECTOR_TABLE_LOCATION 0x8000
#define SPIFLASH_SECTOR_OFFSET 54     // FIXME
#define SPIFLASH_SECTORS 1024         // FIXME

#define DISABLE_PWM 1                 // FIXME

#define HAS_STANDARD_GUI    0         // FIXME
#define HAS_ADVANCED_GUI    1
#define HAS_PERMANENT_TIMER 1
#define HAS_TELEMETRY       1
#define HAS_EXTENDED_TELEMETRY 1
#define HAS_LCD_FLIPPED     1
#define HAS_TOUCH           0
#define HAS_RTC             0         // FIXME
#define HAS_VIBRATINGMOTOR  1
#define HAS_DATALOG         0         // FIXME
#define SUPPORT_SCANNER         0
#define HAS_LAYOUT_EDITOR   1
#define HAS_EXTRA_SWITCHES  0
#define HAS_SWITCHES_NOSTOCK 1
#define HAS_EXTRA_BUTTONS   0
#define HAS_EXTRA_POTS      OPTIONAL
#define HAS_MULTIMOD_SUPPORT 1
#define HAS_VIDEO           0
#define HAS_4IN1_FLASH      0
#define HAS_EXTENDED_AUDIO  0         // FIXME
#define HAS_AUDIO_UART5     0         // FIXME
#define HAS_MUSIC_CONFIG    0         // FIXME
#define USE_4BUTTON_MODE    0
#define HAS_AUDIO_UART      0
#define HAS_OLED_DISPLAY    0
#define HAS_HARD_POWER_OFF  0
#define HAS_PWR_SWITCH_INVERTED 1

#define SUPPORT_STACKDUMP       0

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

#define INP_HAS_CALIBRATION 6

/* Compute voltage from y = 2.1592x + 0.2493 */
#define VOLTAGE_NUMERATOR 216
#define VOLTAGE_OFFSET    249

#include "hardware.h"
#include "x9d_ports.h"

// FIXME: These should come from a common source
void ADC_Init(void);
void ADC_StartCapture();

void CLOCK_ClearMsecCallback(int MsecCallback);
u32 SOUND_Callback();

extern void PROTO_Stubs(int);
// ADC defines
#define NUM_ADC_CHANNELS (INP_HAS_CALIBRATION + 2)  // Inputs + Temprature + Voltage
extern volatile u16 adc_array_raw[NUM_ADC_CHANNELS];
void ADC_Filter();
#ifndef LCD_ForceUpdate
static inline void LCD_ForceUpdate() {}
#endif


#endif  // _T12_TARGET_H_
