#ifndef _ENABLE_FATFS_H_
#define _ENABLE_FATFS_H_

#include "FatFs/ff.h"

    typedef struct {
        FATFS fat;
        FIL fil;
    } DRIVE;
    #define FSHANDLE FIL

    inline static int fs_mount(FATFS *fat) {
        int res = f_mount(fat, "", 1);
        if (res == FR_OK)
            f_chdir("/");
        return res;
    }
    #define fs_is_initialized(x)            (((char *)(x))[0] != 0)
    #define fs_add_file_descriptor(x, y)    FS_Mount(x, y)
    #define fs_open(fp, path, flags, mode)   f_open((fp), (path), (mode) == O_RDONLY ? FA_READ : FA_WRITE | ((flags) & O_CREAT ? FA_CREATE_ALWAYS : 0))
    #define fs_read                   f_read
    #define fs_lseek                  f_lseek
    #define fs_opendir                f_opendir
    #define fs_readdir                f_readdir
    #define fs_write                  f_write
    #define fs_maximize_file_size()   if (0) {}
    #define fs_ltell                  f_tell
    #define fs_get_drive_num(x)       0
    #define fs_set_drive_num(x, num)  if (0) {}
    #define fs_is_open(x)             (x)->obj.fs
    #define fs_close                  f_close
    #define fs_filesize(x)            (x)->obj.objsize
    #define fs_switchfile(x)          (void)(x)
    static inline void fs_init(FSHANDLE * fh, const char *drive) {
        (void)fh;
        (void)drive;
    }
#endif  // _ENABLE_FATFS_H
