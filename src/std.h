#ifndef _STD_H_
#define _STD_H_

/* the following defines allow control over how stdio/stdlib functions are handles */
#ifndef EMULATOR
#if (! defined BUILDTYPE_DEV)
  //Use this instead of printf(args...) because this will avoid
  //compile warnings
  #define printf if(0) printf
#else
//  #define printf iprintf
#endif
//#define sprintf siprintf
//#define snprintf sniprintf
//#define fprintf fiprintf
//#define fread(a, b, c, d) 0
//#define fopen(a, b) -1
//#define fclose if(0) fclose
//#define fgets(x, y, z) 0
//#define setbuf if(0) setbuf
#endif
#endif
