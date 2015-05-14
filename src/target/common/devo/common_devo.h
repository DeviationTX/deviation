#ifndef _COMMON_DEVO_H_
#define _COMMON_DEVO_H_

#ifndef FILE_SIZE
#include "petit_fat.h"
#define FILE_SIZE sizeof(FATFS)
#endif

#define fat_mount      pf_mount
#define fat_open       pf_open
#define fat_read       pf_read
#define fat_lseek      pf_lseek
#define fat_close      pf_close
#define fat_opendir    pf_opendir
#define fat_readdir    pf_readdir
#define fat_write      pf_write

#include "ports.h"

//Devo does drawing with LCD_Stop so ForceUpdate isn't needed
#ifndef LCD_ForceUpdate
static inline void LCD_ForceUpdate() {}
#endif

#endif //_COMMON_DEVO_H_
