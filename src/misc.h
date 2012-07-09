#ifndef _MISC_H_
#define _MISC_H_
#include "target.h"

void Delay(u32 count);
u32 Crc(void *buffer, u32 size);
void Hang();
#endif
