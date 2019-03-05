#ifndef _ENABLE_PETIT_FAT_H_
#define _ENABLE_PETIT_FAT_H_

    #include "petit_fat/petit_fat.h"

    typedef union {
        FATFS fat;
        FATFS fil;
    } DRIVE;
    #define FSHANDLE FATFS

    #define fs_mount                        pf_mount
    #define fs_is_initialized(x)            (((char *)(x))[0] != 0)
    #ifdef O_CREAT
        inline int fs_open(void *ptr, char *file, unsigned flags, int mode) {
            (void) ptr;
            (void) mode;
            int ret = pf_open(file);
            if (flags & O_CREAT)
                pf_maximize_file_size();
            return ret;
        }
    #endif
    #define fs_open(ptr, path, flags, mode)   pf_open(path)
    #define fs_read(r, ptr, len, br)  pf_read(ptr, len, (WORD *)(br))
    #define fs_lseek(r, ptr)          pf_lseek(ptr)
    #define fs_opendir                pf_opendir
    #define fs_readdir                pf_readdir
    #define fs_write(r, ptr, len, bw) pf_write(ptr, len, (WORD *)(bw))
    #define fs_switchfile             pf_switchfile
    #define fs_maximize_file_size     pf_maximize_file_size
    #define fs_ltell(x)               (x)->fptr
    #define fs_get_drive_num(x)       (x)->pad1
    #define fs_set_drive_num(x,num)   (x)->pad1 = num
    #define fs_is_open(x)             ((x)->flag & FA_OPENED)
    #define fs_close(x)               (x)->flag = 0
    #define fs_filesize(x)            (x)->fsize
    int FS_Mount(void *FAT, const char *drive);
    static inline void fs_init(FSHANDLE * fh, const char *drive)
    {
        if (!fs_is_initialized(fh))
            FS_Mount(fh, drive);
    }
#endif //_ENABLE_PETIT_FAT_H_
