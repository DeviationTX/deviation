#ifndef _STD_H_
#define _STD_H_

/* the following defines allow control over how stdio/stdlib functions are handles */
#ifndef EMULATOR

#include <stdarg.h>
void tfp_printf(char *fmt, ...);
void tfp_sprintf(char* s,char *fmt, ...);
void tfp_fprintf(FILE *fh,char *fmt, ...);

#if (! defined BUILDTYPE_DEV)
  //Use this instead of printf(args...) because this will avoid
  //compile warnings
  #define printf if(0) iprintf
#else
  //#define printf iprintf
  #define printf tfp_printf
#endif

//#define sprintf siprintf
#define sprintf tfp_sprintf
#define snprintf sniprintf
//#define fprintf fiprintf
#define fprintf tfp_fprintf
//#define fread(a, b, c, d) 0
//#define fopen(a, b) -1
//#define fclose if(0) fclose
//#define fgets(x, y, z) 0
//#define setbuf if(0) setbuf
#endif
#endif
