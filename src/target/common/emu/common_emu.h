#ifndef _COMMON_EMU_H_
#define _COMMON_EMU_H_

//FAT is unused for the emulator
#define FILE_SIZE 1

//This is a trick to only enable this function in the emu and to use an inline version for devo
#define LCD_ForceUpdate LCD_ForceUpdate
#endif
