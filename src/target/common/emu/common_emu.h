#ifndef _COMMON_EMU_H_
#define _COMMON_EMU_H_

//FAT is unused for the emulator
#if EMULATOR == USE_NATIVE_FS
    #define FATSTRUCT_SIZE 1
    #define fs_is_initialized(x) x
    #define fs_add_file_descriptor(x, y) (void)NULL
#endif
//This is a trick to only enable this function in the emu and to use an inline version for devo
#define LCD_ForceUpdate LCD_ForceUpdate
#endif
