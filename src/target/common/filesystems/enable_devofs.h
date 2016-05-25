#ifndef _ENABLE_DEVOFS_H_
#define _ENABLE_DEVOFS_H_

    #include "devofs/devofs.h"

    #define FATSTRUCT_SIZE sizeof(FATFS)

    #define fs_mount      df_mount
    #define fs_open       df_open
    #define fs_read       df_read
    #define fs_lseek      df_lseek
    #define fs_opendir    df_opendir
    #define fs_readdir    df_readdir
    #define fs_write      df_write
    #define fs_switchfile df_switchfile
    #define fs_maximize_file_size()         if(0) {}
    #define fs_set_drive_num(x,num)         if(0) {}
    #define fs_get_drive_num(x)             0
    #define fs_is_open(x)                   ((x)->file_cur_pos != -1)
    #define fs_close(x)                     df_switchfile(x); df_close()
    #define fs_filesize(x)                  (((x)->file_header.size1 << 8) | (x)->file_header.size2)
    #define fs_ltell(x)                     ((x)->file_cur_pos)
    #define fs_is_initialized(x)            (((FATFS *)(x))->start_sector != ((FATFS *)(x))->compact_sector)
    #define fs_add_file_descriptor(x, y)    df_add_file_descriptor((FATFS *)(x))

#endif //_ENABLE_DEVOFS_H_
