#ifndef _ENABLE_NATIVE_FS_H_
#define _ENABLE_NATIVE_FS_H_
    typedef struct {
        int dummy;
    }FATFS;
    #define fs_is_initialized(x) x
    #define fs_add_file_descriptor(x, y) (void)NULL
#endif
