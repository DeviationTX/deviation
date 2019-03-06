

#if defined(EMULATOR) && EMULATOR == USE_NATIVE_FS
    #include "enable_native_fs.h"
#elif defined USE_DEVOFS && USE_DEVOFS == 1
    #include "enable_devofs.h"
#else
    #include "enable_petit_fat.h"
#endif

#if (defined(HAS_4IN1_FLASH) && HAS_4IN1_FLASH) || defined(EMULATOR)
    #define FLASHTYPE FLASHTYPE_SPI
    #define HAS_FLASH_DETECT
    #define SUPPORT_STACKDUMP 1
#else
    #define FLASHTYPE FLASHTYPE_MCU
    #define SUPPORT_STACKDUMP 0
#endif

#define USE_4BUTTON_MODE    1
#define HAS_HARD_POWER_OFF  1
#define HAS_PWR_SWITCH_INVERTED 0

#include "hardware.h"
