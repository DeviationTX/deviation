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
int _open_r (void *r, const char *file, int flags, int mode);
int _close_r (void *r, int fd);
int _read_r (void *r, int fd, char * ptr, int len);
int _write_r (void *r, int fd, char * ptr, int len);
int _lseek_r (void *r, int fd, int ptr, int dir);

FILE *devo_fopen(const char *path, const char *mode)
{
    int flags = (mode && *mode == 'w') ? O_CREAT : 0;
    int _mode = flags ? O_WRONLY : O_RDONLY;
        
    long fd = _open_r (NULL, path, flags, _mode);
    fd++;
    return (void *)fd;
}

int devo_fclose(FILE *fp)
{
    long fd = (long)fp - 1;
    return _close_r(NULL, fd);
}

int devo_fseek(FILE *stream, long offset, int whence)
{
    long fd = (long)stream - 1;
    return _lseek_r(NULL, fd, offset, whence);
}

int devo_fputc(int c, FILE *stream) {
    long fd = (long)stream - 1;
    char ch = c;
    return (_write_r(NULL, fd, &ch, 1) == -1) ? -1 : c;
}
extern unsigned char _stop_on_cr;
char *devo_fgets(char *s, int size, FILE *stream)
{
    long fd = (long)stream - 1;
    _stop_on_cr = 1;
    int r = _read_r(NULL, fd, s, size-1);
    _stop_on_cr = 0;
    if(r <= 0)
        return NULL;
    s[r] = '\0';
    return s;
}
size_t devo_fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    long fd = (long)stream - 1;
    int r = _read_r(NULL, fd, ptr, size * nmemb);
    if (r <= 0)
        return 0;
    return nmemb;
}
void devo_setbuf(FILE *stream, char *buf)
{
    (void)stream;
    (void)buf;
}
