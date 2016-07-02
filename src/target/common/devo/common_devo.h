#ifndef _COMMON_DEVO_H_
#define _COMMON_DEVO_H_

#ifndef FATSTRUCT_SIZE
    #if defined USE_DEVOFS && USE_DEVOFS == 1
        #include "enable_devofs.h"
    #else
        #include "enable_petit_fat.h"
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

#endif //_COMMON_DEVO_H_
