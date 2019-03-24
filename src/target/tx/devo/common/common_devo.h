#ifndef _COMMON_DEVO_H_
#define _COMMON_DEVO_H_

#ifndef _TARGET_H_
#error "Don't include target_defs.h directly, include target.h instead."
#endif

#if defined(EMULATOR) && EMULATOR == USE_NATIVE_FS
    #include "enable_native_fs.h"
#elif defined USE_DEVOFS && USE_DEVOFS == 1
    #include "enable_devofs.h"
#else
    #include "enable_petit_fat.h"
#endif

#ifndef SUPPORT_STACKDUMP
    #if !defined(USE_DEVOFS) || USE_DEVOFS != 1
        #define SUPPORT_STACKDUMP 1
    #else
        #define SUPPORT_STACKDUMP 0
    #endif
#endif
#include "ports.h"

//Devo does drawing with LCD_Stop so ForceUpdate isn't needed
#ifndef LCD_ForceUpdate
static inline void LCD_ForceUpdate() {}
#endif

#ifndef USE_4BUTTON_MODE
    #define USE_4BUTTON_MODE 0
#endif

#define FLASHTYPE FLASHTYPE_SPI

#ifndef HAS_HARD_POWER_OFF
#define HAS_HARD_POWER_OFF 0
#endif

#ifndef HAS_PWR_SWITCH_INVERTED
#define HAS_PWR_SWITCH_INVERTED 0
#endif

#ifndef HAS_OLED_DISPLAY
    #define HAS_OLED_DISPLAY 0
    #define TXTYPE ""
#endif
#endif //_COMMON_DEVO_H_
