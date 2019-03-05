#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include "common.h"

#if EMULATOR == USE_NATIVE_FS
void SPIFlash_Init() {}
void fempty(FILE *fh)
{
    fseek(fh, 0, SEEK_END);
    long pos = ftell(fh);
    int fd = fileno(fh);
    ftruncate(fd, 0);
    ftruncate(fd, pos);
    fseek(fh, 0, SEEK_SET);
}

int FS_Init() {
    printf("Changing directory to: '%s'\n", FILESYSTEM_DIR);
    return !chdir(FILESYSTEM_DIR);
}

int FS_Mount(void *FAT, const char *drive) {
    (void)FAT;
    (void)drive;
    return 1;
}

static DIR *dh;
int FS_OpenDir(const char *path)
{
    dh = opendir(path);
    return (dh != NULL);
}
int FS_ReadDir(char *path)
{
    struct dirent *dir = readdir(dh);
    if (! dir)
        return 0;
    strlcpy(path, dir->d_name, 13);
    return 1;
}

void FS_CloseDir() {
    closedir(dh);
}
#endif //USE_NATIVE_FS
