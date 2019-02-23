#ifndef _COMMON_EMU_H_
#define _COMMON_EMU_H_

#ifndef _TARGET_H_
#error "Don't include target_defs.h directly, include target.h instead."
#endif

#define TARGET_PRIORITY \
    TIMER_ENABLE
//This is a trick to only enable this function in the emu and to use an inline version for devo
#define LCD_ForceUpdate LCD_ForceUpdate
#endif
