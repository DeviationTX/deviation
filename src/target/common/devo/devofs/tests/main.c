#include "devofs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

char image_file[1024];
static DIR   dir;
int FS_OpenDir(const char *path)
{
    
    FRESULT res = df_opendir(&dir, path);
    printf("Opendir: %d\n", res);
    return (res == FR_OK);
}

int FS_ReadDir(char *path)
{
    FILINFO fi;
    if (df_readdir(&dir, &fi) != FR_OK || ! fi.fname[0])
        return 0;
    printf("Read: %s %d\n", fi.fname, fi.fattrib);
    strncpy(path, fi.fname, 13);
    return fi.fattrib & AM_DIR ? 2 : 1;
}

int FS_ReadData(char *data)
{
    int res;
    char *ptr = data;
    while(1) {
        u16 bytes;
        res = df_read(ptr, 1024, &bytes);
        printf("df_read: %d %dbytes\n", res, bytes);
        ptr += bytes;
        if(res || bytes != 1024)
            break;
    }
    *ptr = 0;
    return ptr - data;
}

int main(int argc, char *argv[]) {
	FATFS fat;
	FILE *fh;
	unsigned char data[1024 * 1024];
        unsigned char *ptr = data;
        unsigned int len;
	int res;
        if (argc != 2) {
            printf("%s <devofs image>\n", argv[1]);
            exit(0);
        }
        sprintf(image_file, "%s.tmp", argv[1]);
        char cmd[1024];
        sprintf(cmd, "/bin/cp %s %s", argv[1], image_file);
        system(cmd);
	res = df_mount(&fat);
	printf("df_mount: %d\n", res);
        FS_OpenDir("media");
        while((res = FS_ReadDir(data))) {
            printf("ReadDir: %d: %s\n", res, data);
            char filename[256];
            sprintf(filename, "media/%s", data);
            res = df_open(filename, 0);
            printf("df_open: %d\n", res);
            if (res) {
                continue;
            }
            FS_ReadData(data);
            printf("%s\n", data);
        }
        res = df_open("template/foo", 0);
        printf("df_open(template/foo): %d\n", res);
        if(res == 0) {
            int len = FS_ReadData(data);
            int fh;
            fh = open("foo.tmp", O_WRONLY | O_CREAT);
            write(fh, data, len);
            close(fh);
        }
        res = df_open("template/foo", O_WRONLY);
        printf("df_open('template/foo', O_WRONLY): %d\n", res);
        int i;
        for(i = 0; i < 128; i++) {
            int len = 8192 / 256;
            memset(data, 0xff-i, len);
            u16 result;
            res = df_write(data, len, &result);
            printf("df_write(%d) : %d %d\n", res, len, result);
            if (res || result != len)
                break;
        }
        res = df_open("template/foo", 0);
        printf("df_open(template/foo): %d\n", res);
        if(res == 0) {
            int len = FS_ReadData(data);
            int fh;
            fh = open("foo.tmp1", O_WRONLY | O_CREAT);
            write(fh, data, len);
            close(fh);
        }
        res = df_compact();
        printf("df_compact(): %d\n", res);
#if 0
	res = df_open("model/model1.ini");
	printf("df_open: %d\n", res);
	df_maximize_file_size();
	res = df_open("bmp/devo8.bmp");
	printf("df_open: %d\n", res);
        while(1) {
            WORD b;
            res = df_read(ptr, 0x1000, &b);
            len += b;
            if(res || b != 0x1000)
                break;
            ptr+= 0x1000;
        }
	printf("df_read: %d\n", res);
	printf("Read %d\n", len);
	fh = fopen("out.bmp", "w");
	fwrite(data, len, 1, fh);
	fclose(fh);
#endif
	return 0;
}
        
