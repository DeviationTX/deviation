#ifndef _COMMON_EMU_H_
#define _COMMON_EMU_H_

#ifndef _TARGET_H_
#error "Don't include target_defs.h directly, include target.h instead."
#endif

//FAT is unused for the emulator
//#if EMULATOR == USE_NATIVE_FS
//    typedef struct {
//        int dummy;
//    }FATFS;
//    #define fs_is_initialized(x) x
//    #define fs_add_file_descriptor(x, y) (void)NULL
//#endif
//This is a trick to only enable this function in the emu and to use an inline version for devo
#define LCD_ForceUpdate LCD_ForceUpdate
#endif
