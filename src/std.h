#ifndef _STD_H_
#define _STD_H_

/* the following defines allow control over how stdio/stdlib functions are handles */
#ifndef EMULATOR

#define USE_OWN_STDIO 1
#ifndef USE_OWN_PRINTF
    #define USE_OWN_PRINTF 1
#endif
#if USE_OWN_STDIO
    FILE *devo_fopen2(void *, const char *path, const char *mode);
    int devo_fclose(FILE *fp);
    int devo_fseek(FILE *stream, long offset, int whence);
    int devo_fputc(int c, FILE *stream);
    char *devo_fgets(char *s, int size, FILE *stream);
    size_t devo_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
    void devo_setbuf(FILE *stream, char *buf);

    #undef stdout
    #define stdout (void *)(1L)
    #undef stderr
    #define stderr (void *)(2L)
    #define fopen(p, m)  devo_fopen2(NULL, p, m)
    #define fopen2(fat, p, m)  devo_fopen2(fat, p, m)
    #define fclose devo_fclose
    #define fseek devo_fseek
    #define fputc devo_fputc
    #undef putc
    #define putc  devo_fputc
    #define fgets devo_fgets
    #define gets  devo_fgets
    #define fread devo_fread
    #define setbuf devo_setbuf
#endif //USE_OWN_STDIO

#if USE_OWN_PRINTF
    #include <stdarg.h>
    void tfp_printf(const char *fmt, ...);
    void tfp_sprintf(char* s,const char *fmt, ...);
    void tfp_fprintf(FILE *fh,const char *fmt, ...);
    #define sprintf tfp_sprintf
    #define fprintf tfp_fprintf
    #if (! defined BUILDTYPE_DEV)
        //Use this instead of printf(args...) because this will avoid
        //compile warnings
        #define printf if(0) tfp_printf
    #else
        #define printf tfp_printf
    #endif  //BUILDTYPE_DEV
#endif //USE_OWN_PRINTF
#else //EMULATOR
    #define fopen2(fat, p, m) fopen(p, m)
#endif //EMULATOR
#endif //_STD_H_
