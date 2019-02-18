#ifndef _ENABLE_NATIVE_FS_H_
#define _ENABLE_NATIVE_FS_H_
    #define FIL FATFS
    typedef struct {
        int dummy;
    }FATFS;
    #define fs_is_initialized(x) x
    #define fs_add_file_descriptor(x, y) (void)NULL
#endif
