#ifndef _TARGET_H_
#define _TARGET_H_

/* UART & Debug */
void UART_Initialize();
void UART_Stop();

/* Power functions */
void PWR_Init(void);

static inline int _ltell_r(void *fh) {(void)fh; return -1;}
static inline char *__f_gets(char *s, int size, void *stream) {(void)s; (void)size; (void)stream; return NULL;}
static inline int FS_Mount(void *FAT, const char *drive) {(void)FAT; (void)drive; return 1;}
#define _f_gets __f_gets

//Load target-specific include
#include "target_defs.h"
#endif
