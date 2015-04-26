#ifndef _STD_H_
#define _STD_H_

/* the following defines allow control over how stdio/stdlib functions are handles */
#define strncpy "BAD_FUNC"  //Do not allow any use of strncpy (use strlcpy instead)
#ifndef USE_OWN_PRINTF
    #define USE_OWN_PRINTF 1
#endif

#ifndef EMULATOR
    #ifndef USE_OWN_STDIO
        #define USE_OWN_STDIO 1
    #endif
#else
    #ifndef USE_OWN_STDIO
        #define USE_OWN_STDIO 0
    #endif
    #define fopen2(fat, p, m) fopen(p, m)
    #define finit if(0) FS_Mount
    void fempty(FILE *fh);
#endif //EMULATOR

#if USE_OWN_STDIO
    FILE *devo_fopen2(void *, const char *path, const char *mode);
    int devo_fclose(FILE *fp);
    int devo_fseek(FILE *stream, long offset, int whence);
    int devo_fputc(int c, FILE *stream);
    char *devo_fgets(char *s, int size, FILE *stream);
    size_t devo_fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
    size_t devo_fwrite(void *ptr, size_t size, size_t nmemb, FILE *stream);
    void devo_setbuf(FILE *stream, char *buf);
    long devo_ftell(FILE *stream);
    void devo_finit(void *FAT, const char *str);
    void fempty(FILE *fh);

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
    #define fwrite devo_fwrite
    #define ftell devo_ftell
    #define finit devo_finit
    #define setbuf devo_setbuf
#endif //USE_OWN_STDIO

#if USE_OWN_PRINTF
    #include <stdarg.h>
    void tfp_printf(const char *fmt, ...);
    void tfp_sprintf(char* s,const char *fmt, ...);
    void tfp_snprintf(char* s, int len, const char *fmt, ...);
    void tfp_fprintf(FILE *fh,const char *fmt, ...);
    #define sprintf tfp_sprintf
    #define snprintf tfp_snprintf
    #define fprintf tfp_fprintf
    #if ! defined BUILDTYPE_DEV  && ! DEBUG_WINDOW_SIZE
        //Use this instead of printf(args...) because this will avoid
        //compile warnings
        #define printf if(0) tfp_printf
    #else
        #define printf tfp_printf
    #endif  //BUILDTYPE_DEV
#endif //USE_OWN_PRINTF
#endif //_STD_H_
