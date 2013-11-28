/*
File: printf.c

Copyright (C) 2004  Kustaa Nyholm

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Code originally from: http://www.sparetimelabs.com/tinyprintf/index.html
*/

#define USE_OWN_PRINTF 0
#include "common.h"
#include <stdarg.h>

typedef void (*putcf) (void*,char);

#define stdout_putp NULL
static void stdout_putf (void *p, char c) {
    (void)p;
    //if (c == '\n')
    //    usart_send_blocking(USART1, '\r');
    //usart_send_blocking(USART1, c);
    fputc(c, stdout);
};
static void fputf (void *p, char c) {
    fputc(c, (FILE *)p);
};
static void putcp(void* p,char c)
{
    *(*((char**)p))++ = c;
    if(c != '\0')
        *(*((char**)p)) = '\0';
}

static void ui2a(unsigned int num, unsigned int base, int uc,char * bf)
    {
    int n=0;
    unsigned int d=1;
    while (num/d >= base)
        d*=base;        
    while (d!=0) {
        int dgt = num / d;
        num%= d;
        d/=base;
        if (n || dgt>0 || d==0) {
            *bf++ = dgt+(dgt<10 ? '0' : (uc ? 'A' : 'a')-10);
            ++n;
            }
        }
    *bf=0;
    }

static void i2a (int num, char * bf)
    {
    if (num<0) {
        num=-num;
        *bf++ = '-';
        }
    ui2a(num,10,0,bf);
    }

static int a2d(char ch)
    {
    if (ch>='0' && ch<='9') 
        return ch-'0';
    else if (ch>='a' && ch<='f')
        return ch-'a'+10;
    else if (ch>='A' && ch<='F')
        return ch-'A'+10;
    else return -1;
    }

static char a2i(char ch, const char** src,int base,int* nump)
    {
    const char* p= *src;
    int num=0;
    int digit;
    while ((digit=a2d(ch))>=0) {
        if (digit>base) break;
        num=num*base+digit;
        ch=*p++;
        }
    *src=p;
    *nump=num;
    return ch;
    }

static void tfp_format(void* putp,putcf putf,unsigned int buffer_len, const char *fmt, va_list va)
    {
    unsigned int bufsize = 0;
    char bf[12];
    
    char ch;

    void _putc(char ch)
    {
        if (buffer_len && bufsize + 1 >= buffer_len)
            return;
        putf(putp, ch);
        bufsize++;
    }
    void _putchw(int n, char z, char* bf)
    {
        char fc=z? '0' : ' ';
        char* p=bf;
        unsigned int i;
        while (*p++) {
            if (n > 0)
                n--;
        }
        unsigned int len = (p - bf) + n - 1;
        if (buffer_len && buffer_len - bufsize - 1 < len)
            len = buffer_len - bufsize - 1;
        for (i = 0; i < len; i++) {
            if(n) {
                putf(putp,fc);
                n--;
            } else {
                putf(putp, *bf++);
            }
        }
        bufsize += len;
    }


    while ((ch=*(fmt++))) {
        if (ch!='%') 
            _putc(ch);
        else {
            char lz=0;
#ifdef  PRINTF_LONG_SUPPORT
            char lng=0;
#endif
            int w=0;
            ch=*(fmt++);
            if (ch=='0') {
                ch=*(fmt++);
                lz=1;
                }
            if (ch>='0' && ch<='9') {
                ch=a2i(ch,&fmt,10,&w);
                }
            if (ch=='l') {
                ch=*(fmt++);
            }
            switch (ch) {
                case 0: 
                    goto abort;
                case 'u' : {
#ifdef  PRINTF_LONG_SUPPORT
                    if (lng)
                        uli2a(va_arg(va, unsigned long int),10,0,bf);
                    else
#endif
                    ui2a(va_arg(va, unsigned int),10,0,bf);
                    _putchw(w,lz,bf);
                    break;
                    }
                case 'd' :  {
#ifdef  PRINTF_LONG_SUPPORT
                    if (lng)
                        li2a(va_arg(va, unsigned long int),bf);
                    else
#endif
                    i2a(va_arg(va, int),bf);
                    _putchw(w,lz,bf);
                    break;
                    }
                case 'x': case 'X' : 
#ifdef  PRINTF_LONG_SUPPORT
                    if (lng)
                        uli2a(va_arg(va, unsigned long int),16,(ch=='X'),bf);
                    else
#endif
                    ui2a(va_arg(va, unsigned int),16,(ch=='X'),bf);
                    _putchw(w,lz,bf);
                    break;
                case 'c' : 
                    _putc((char)(va_arg(va, int)));
                    break;
                case 's' : 
                    _putchw(w,0,va_arg(va, char*));
                    break;
                case '%' :
                    _putc(ch);
                default:
                    break;
                }
            }
        }
    abort:;
    }


void tfp_printf(const char *fmt, ...)
    {
    va_list va;
    va_start(va,fmt);
    tfp_format(stdout_putp, stdout_putf, 0, fmt, va);
    va_end(va);
    }



void tfp_sprintf(char* s, const char *fmt, ...)
    {
    va_list va;
    va_start(va,fmt);
    tfp_format(&s, putcp, 0, fmt, va);
    va_end(va);
    }

void tfp_snprintf(char* s, unsigned int len, const char *fmt, ...)
    {
    va_list va;
    va_start(va,fmt);
    tfp_format(&s, putcp, len, fmt, va);
    va_end(va);
    }

void tfp_fprintf(FILE* fh, const char *fmt, ...)
    {
    va_list va;
    va_start(va,fmt);
    tfp_format(fh, fputf, 0, fmt, va);
    va_end(va);
    }
