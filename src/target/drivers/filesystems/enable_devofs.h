#ifndef _ENABLE_DEVOFS_H_
#define _ENABLE_DEVOFS_H_

    #include "devofs/devofs.h"

    typedef union {
        FATFS fat;
        FATFS fil;
    } DRIVE;
    #define FSHANDLE FATFS

    #define fs_mount      df_mount
    #define fs_open(ptr, path, flags, mode)   df_open(path, flags)
    #define fs_read(r, ptr, len, br)          df_read(ptr, len, (u16 *)(br))
    #define fs_lseek(r, ptr)                  df_lseek(ptr)
    #define fs_opendir                        df_opendir
    #define fs_readdir                        df_readdir
    #define fs_write(r, ptr, len, bw)         df_write(ptr, len, (u16 *)(bw))
    #define fs_switchfile                     df_switchfile
    #define fs_maximize_file_size()           if (0) {}
    #define fs_set_drive_num(x, num)          if (0) {}
    #define fs_get_drive_num(x)               0
    #define fs_is_open(x)                     ((x)->file_cur_pos != -1)
    #define fs_close(x)                       df_switchfile(x); df_close()
    #define fs_filesize(x)                    (((x)->file_header.size1 << 8) | (x)->file_header.size2)
    #define fs_ltell(x)                       ((x)->file_cur_pos)
    #define fs_is_initialized(x)              (((FATFS *)(x))->start_sector != ((FATFS *)(x))->compact_sector)
    static inline void fs_init(FSHANDLE * fh, const char *drive)
    {
        (void)drive;
        if (!fs_is_initialized(fh))
            df_add_file_descriptor((FATFS *)fh);
    }

#endif //_ENABLE_DEVOFS_H_
