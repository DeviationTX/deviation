
#include <stdint.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* File function return code (FRESULT) */

typedef enum {
	FR_OK = 0,			/* 0 */
	FR_DISK_ERR,		/* 1 */
	FR_NOT_READY,		/* 2 */
	FR_NO_FILE,			/* 3 */
	FR_NO_PATH,			/* 4 */
	FR_NOT_OPENED,		/* 5 */
	FR_NOT_ENABLED,		/* 6 */
	FR_NO_FILESYSTEM	/* 7 */
} FRESULT;

enum {AM_FILE = 0x00, AM_DIR = 0x01};

struct file_header {
    u8 type;
    u8 parent_dir;
    char name[11];
    u8 size1; //also id
    u8 size2;
    u8 size3;
};
typedef struct {
    int start_sector;
    int compact_sector;
    int file_addr;
    int file_cur_pos;
    int parent_dir;
    struct file_header file_header;
} FATFS;

#define DIR FATFS

typedef struct {
	u32	fsize;		/* File size */
        u8      fattrib;
        char    fname[13];      /* File name */
} FILINFO;

FRESULT df_mount (FATFS*);						/* Mount/Unmount a logical drive */
FRESULT df_switchfile (FATFS *);
FRESULT df_open (const char*, unsigned flags);					/* Open a file */
FRESULT df_close ();
FRESULT df_read (void*, u16, u16*);			/* Read data from the open file */
FRESULT df_write (const void*, u16, u16*);	/* Write data to the open file */
FRESULT df_maximize_file_size();
FRESULT df_lseek (u32);						/* Move file pointer of the open file */
FRESULT df_opendir (DIR*, const char*);			/* Open a directory */
FRESULT df_readdir (DIR*, FILINFO*);			/* Read a directory item from the open directory */
FRESULT df_mkdir(const char *name);
FRESULT df_unlink(const char *name);
FRESULT df_stat(FILINFO *fi);
FRESULT df_compact ();
