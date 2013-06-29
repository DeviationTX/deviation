/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <fcntl.h>
#define FILE void
long _open_r (void *r, const char *file, int flags, int mode);
int _close_r (void *r);
int _read_r (void *r, char * ptr, int len);
int _write_r (void *r, char * ptr, int len);
int _lseek_r (void *r, int ptr, int dir);
int _ltell_r (void *r);
int FS_Mount(void *FAT, const char *drive);

FILE *devo_fopen2(void *r, const char *path, const char *mode)
{
    int flags = (mode && *mode == 'w') ? O_CREAT : 0;
    int _mode = flags ? O_WRONLY : O_RDONLY;
        
    long fd = _open_r (r, path, flags, _mode);
    if (fd <= 0)
        return NULL;
    return (FILE *)fd;
}

int devo_fclose(FILE *fp)
{
    return _close_r(fp);
}

int devo_fseek(FILE *stream, long offset, int whence)
{
    return _lseek_r(stream, offset, whence);
}

int devo_ftell(FILE *stream)
{
    return _ltell_r(stream);
}

int devo_fputc(int c, FILE *stream) {
    char ch = c;
    return (_write_r(stream, &ch, 1) == -1) ? -1 : c;
}
extern unsigned char _stop_on_cr;
char *devo_fgets(char *s, int size, FILE *stream)
{
    _stop_on_cr = 1;
    int r = _read_r(stream, s, size-1);
    _stop_on_cr = 0;
    if(r <= 0)
        return NULL;
    s[r] = '\0';
    return s;
}
size_t devo_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    int r = _read_r(stream, ptr, size * nmemb);
    if (r <= 0)
        return 0;
    return nmemb;
}
size_t devo_fwrite(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    int r = _write_r(stream, ptr, size * nmemb);
    if (r <= 0)
        return 0;
    return nmemb;
}

void devo_finit(void *FAT, const char *drive)
{
    if(! ((char *)FAT)[0])
        FS_Mount(FAT, drive);
}

void fempty(FILE *stream)
{
    long pos = 0;
    long len = devo_fseek(stream, 0, 2 /* SEEK_END */);
    devo_fseek(stream, 0, 0 /*SEEK_SET*/);
    while(pos < len) {
        devo_fputc(0x00, stream);
        pos += 4096;
        devo_fseek(stream, pos, 0 /*SEEK_SET*/);
    }
    devo_fseek(stream, 0, 0 /*SEEK_SET*/);
}

void devo_setbuf(FILE *stream, char *buf)
{
    (void)stream;
    (void)buf;
}
