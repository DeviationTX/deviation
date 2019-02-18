#ifndef _FATIO_H_
#define _FATIO_H_

#ifndef FILE_SIZE
#include "FatFs/ff.h"
#define FILE_SIZE sizeof(FIL)
#endif

#define fat_mount      f_mount
#define fat_open       f_open
#define fat_read       f_read
#define fat_lseek      f_lseek
#define fat_close      f_close
#define fat_opendir    f_opendir
#define fat_readdir    f_readdir
#define fat_stat       f_stat
#define fat_write      f_write
#define fat_getfree    f_getfree
#define fat_truncate   f_truncate
#define fat_sync       f_sync
#define fat_unlink     f_unlink

#define _f_gets f_gets
#endif
