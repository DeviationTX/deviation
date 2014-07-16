#define PROTO_HAS_CYRF6936
#define PROTO_HAS_A7105
#define PROTO_HAS_CC2500

#define MODULE_CALLTYPE

#include <stdint.h>
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "../../std.h"
#include "target.h"

#define CHAN_MAX_VALUE 10000
#define CHAN_MIN_VALUE -10000
#define _tr(x) x
#define _tr_noop(x) x

extern s32 Channels[10];

struct limit {
    u8 flags;
    u8 failsafe;
};

#define CH_FAILSAFE_EN 1

#define PROTODEF(proto, module, map, cmd, name) proto,
enum Protocols {
    PROTOCOL_NONE,
    #include "../../protocol/protocol.h"
    PROTOCOL_COUNT,
};
#undef PROTODEF

enum TxPower {
    TXPOWER_100uW,
    TXPOWER_300uW,
    TXPOWER_1mW,
    TXPOWER_3mW,
    TXPOWER_10mW,
    TXPOWER_30mW,
    TXPOWER_100mW,
    TXPOWER_150mW,
    TXPOWER_LAST,
};

enum {
    CYRF6936,
    A7105,
    CC2500,
    NRF24L01,
    MULTIMOD,
    TX_MODULE_LAST,
};
int SPI_ConfigSwitch(unsigned csn_high, unsigned csn_low);
/*
void pabort(const char *s);
void SPI_Init(int idx);
void SPI_WriteRegisterMulti(u8 address, const u8 data[], u8 length);
void SPI_ReadRegisterMulti(u8 address, u8 data[], u8 length);
void SPI_WriteRegister(u8 address, u8 data);
u8 SPI_ReadRegister(u8 address);
*/

void PROTOCOL_SetBindState(int i);
void CLOCK_StopTimer();
void CLOCK_StartTimer(int t, u16 (*_cmd)());
void CLOCK_ResetWatchdog();

u32 CLOCK_getms();
#define PROTOCOL_Init(x) if(0) {};
int PROTOCOL_SticksMoved(int init);

u32 Crc(const void *, int size);
u32 rand32_r(u32 *seed, u8 update); //LFSR based PRNG
u32 rand32(); //LFSR based PRNG


/* Target defs */
void MCU_SerialNumber(u8 *var, int len);
