#ifndef _TARGET_H_
#define _TARGET_H_

#define MAX_PPM_IN_CHANNELS 12
struct mcu_pin {
    u32 port;
    u16 pin;
};

/* UART & Debug */
void UART_Initialize();
void UART_Stop();

/* Power functions */
void PWR_Init(void);

/* PA Control functions */
void PACTL_Init();
void PACTL_SetTxRxMode(int mode);

/* Protocol functions */
void SPI_ProtoInit();

void _usleep(u32 usec);
#define usleep _usleep

static inline char *__f_gets(char *s, int size, void *stream) {(void)s; (void)size; (void)stream; return NULL;}
static inline int FS_Mount(void *FAT, const char *drive) {(void)FAT; (void)drive; return 1;}
#define _f_gets __f_gets

//Load target-specific include
#include "target_defs.h"
#endif
