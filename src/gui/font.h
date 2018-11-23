#ifndef _FONT_H
#define _FONT_H

extern struct FAT FontFAT; // expose to share memory to access FS

#define MAX_FONTSIZE 80

extern u8 FONT_Open(unsigned int idx, const char* name);
extern void FONT_Close();
extern u8 FONT_ReadCharBits(u8 *font, u32 c, u8 *width);
extern u8 FONT_GetHeight();
extern u8 FONT_GetWidth(u32 c);

#endif
