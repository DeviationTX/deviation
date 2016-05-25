#ifndef _ENABLE_PETIT_FAT_H_
#define _ENABLE_PETIT_FAT_H_

    #include "petit_fat/petit_fat.h"

    #define FATSTRUCT_SIZE sizeof(FATFS)

    #define fs_mount                        pf_mount
    #define fs_is_initialized(x)            (((char *)(x))[0] != 0)
    #define fs_add_file_descriptor(x, y)    FS_Mount(x, y)
    #ifdef O_CREAT
        inline int fs_open(char *file, unsigned flags) {
            int ret = pf_open(file);
            if (flags & O_CREAT)
                pf_maximize_file_size();
            return ret;
        }
    #endif
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
#endif //_ENABLE_PETIT_FAT_H_
