// gcc -g -o convert_c_fonts -I../src/screen convert_c_fonts.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "fonts.h"

struct FONT_DEF 
{
	unsigned char width;     	/* Character width for storage         */
	unsigned char height;  		/* Character height for storage        */
        const unsigned int *range;  /* Array containing the ranges of supported characters */
	const unsigned char *font_table;       /* Font table start address in memory  */
};
#define FONTDECL(w, h, range, font, name) {w, h, range, font},
const struct FONT_DEF Fonts[] = {
    #include "fonts.h"
    {0, 0, 0, 0},
};
#undef FONTDECL

int main(int argc, char *argv[])
{
    int idx = atoi(argv[1]);
    const struct FONT_DEF *font = &Fonts[idx];
    unsigned char data[1024*1024];
    unsigned char *ptr = data;
    *ptr++ = font->height;
    int bytes_per_row = ((font->height - 1) / 8) + 1;
    int i = 0;
    int num_chars = 0;
    while(font->range[i]) {
        unsigned int start = font->range[i];
        unsigned int end = font->range[i+1];
        num_chars += 1 + end - start;
        *ptr++ = start & 0xff;
        *ptr++ = (start >> 8) & 0xff;
        *ptr++ = end & 0xff;
        *ptr++ = (end >> 8) & 0xff;
        i += 2;
    }
    *ptr++ = 0;
    *ptr++ = 0;
    *ptr++ = 0;
    *ptr++ = 0;
    int offset;
    offset  = (ptr - data) + 3 + (num_chars * 3);
    *ptr++ = offset & 0xff;
    *ptr++ = (offset >> 8) & 0xff;
    *ptr++ = (offset >> 16) & 0xff;
    int total_width = 0;
    for (i = 0; i < num_chars; i++) {
        int width;
        if (! (font->width & 0x80)) {
            width = font->width * bytes_per_row;
        } else {
            width = font->font_table[i] * bytes_per_row;
        }
        offset += width;
        total_width += width;
        *ptr++ = offset & 0xff;
        *ptr++ = (offset >> 8) & 0xff;
        *ptr++ = (offset >> 16)& 0xff;
    }
    memcpy(ptr, font->font_table + ((font->width & 0x80) ? num_chars : 0), total_width);
    ptr += total_width;
    fwrite(data, ptr - data, 1, stdout);
}
