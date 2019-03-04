#ifndef _ENABLE_NATIVE_FS_H_
#define _ENABLE_NATIVE_FS_H_
    #define FSHANDLE FATFS
    typedef struct {
        int dummy;
    }FATFS;
    typedef union {
        FATFS fat;
        FATFS fil;
    } DRIVE;
    #define fs_is_initialized(x) x
    #define fs_add_file_descriptor(x, y) (void)NULL
#endif
