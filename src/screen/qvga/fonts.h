#ifndef FONTS_H_
#define FONTS_H_

/*
********************************************************************************

	NAME 		: FONT.H
	EXTENDED NAME	: Fonts for graphic LCD based on KS0108 or HD61202
	LAYER		: Application
	AUTHOR		: Stephane REY			  

Ported from Original C51 code to ARM By 
Mukund Deshmukh
email betacomp_ngp@sancharnet.in
http://betacomp.com
Dt 08.06.2006
*/

/* EXTERN Function Prototype(s) */

/* Extern definitions */

#include "target.h"

struct FONT_DEF 
{
	u8 width;     	/* Character width for storage         */
	u8 height;  		/* Character height for storage        */
        u8 first_char;         /* The first character available       */
        u8 last_char;          /* The last character available        */
	const u8 *font_table;       /* Font table start address in memory  */
};
#define WIDTH(x)           (0x7F & x->width)
#define HEIGHT(x)          (x->height)
#define IS_PROPORTIONAL(x) (0x80 & x->width)
extern const struct FONT_DEF Fonts[];

extern const u8 FontSystem3x6[];
extern const u8 FontSystem5x8[];
extern const u8 FontSystem7x8[];
extern const u8 FontCourrier8x12[];
extern const u8 Font8x8thk[];
extern const u8 Font8x8thn[];
extern const u8 Font8x8ord[];

#endif

