#ifndef _COMMON_DEVO_H_
#define _COMMON_DEVO_H_

#ifndef FILE_SIZE
#if defined USE_DEVOFS && USE_DEVOFS == 1
    #include "devofs.h"
#else
    #include "petit_fat.h"
#endif
#define FILE_SIZE sizeof(FATFS)
#endif

#if defined USE_DEVOFS && USE_DEVOFS == 1
    #define fs_mount      df_mount
    #define fs_open       df_open
    #define fs_read       df_read
    #define fs_lseek      df_lseek
    #define fs_opendir    df_opendir
    #define fs_readdir    df_readdir
    #define fs_write      df_write
    #define fs_switchfile df_switchfile
    #define fs_maximize_file_size() if(0) {}
    #define fs_set_drive_num(x,num) if(0) {}
    #define fs_get_drive_num(x) 0
    #define fs_is_open(x) ((x)->file_cur_pos != -1)
    #define fs_close(x) (x)->file_cur_pos = -1
    #define fs_filesize(x) (((x)->file_header.size1 << 8) | (x)->file_header.size2)
    #define fs_ltell(x)   ((x)->file_cur_pos)
#else
    #define fs_mount                  pf_mount
    #define fs_open(str, flags)       pf_open(str)
    #define fs_read                   pf_read
    #define fs_lseek                  pf_lseek
    #define fs_opendir                pf_opendir
    #define fs_readdir                pf_readdir
    #define fs_write                  pf_write
    #define fs_switchfile             pf_switchfile
    #define fs_maximize_file_size     pf_maximize_file_size
    #define fs_ltell(x)               (x)->fptr
    #define fs_get_drive_num(x)       (x)->pad1
    #define fs_set_drive_num(x,num)   (x)->pad1 = num
    #define fs_is_open(x)             ((x)->flag & FA_OPENED)
    #define fs_close(x)               (x)->flag = 0
    #define fs_filesize(x)            (x)->fsize
#endif

#include "ports.h"

//Devo does drawing with LCD_Stop so ForceUpdate isn't needed
#ifndef LCD_ForceUpdate
static inline void LCD_ForceUpdate() {}
#endif

#endif //_COMMON_DEVO_H_
