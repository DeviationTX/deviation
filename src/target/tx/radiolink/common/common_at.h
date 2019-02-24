

#if defined(EMULATOR) && EMULATOR == USE_NATIVE_FS
    #include "enable_native_fs.h"
#elif defined USE_DEVOFS && USE_DEVOFS == 1
    #include "enable_devofs.h"
#else
    #include "enable_petit_fat.h"
#endif

#define FLASHTYPE FLASHTYPE_MCU

#define USE_4BUTTON_MODE 1
#define HAS_HARD_POWER_OFF  1

#include "hardware.h"
