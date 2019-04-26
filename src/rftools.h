#ifndef _RFTOOLS_H_
#define _RFTOOLS_H_

#define MAX_PAYLOAD 32

#if SUPPORT_SCANNER

struct Scanner {
    u8 rssi[255];
    u8 rssi_peak[255];
    u8 chan_min;
    u8 chan_max;
    u8 attenuator;
    u16 averaging;
};

extern struct Scanner Scanner;

#endif  // SUPPORT_SCANNER

enum {
    XN297DUMP_OFF = 0,
    XN297DUMP_MANUAL,
    XN297DUMP_SCAN,
    XN297DUMP_INTERVAL,
};

struct Xn297dump {
    u8 packet[32];
    u8 channel;
    u8 pkt_len;
    u8 crc_valid;
    u8 scan;
    u8 mode;
    u32 interval;
};

extern void RFTOOLS_DumpXN297Packet();
extern void RFTOOLS_InitDumpLog();
extern struct Xn297dump xn297dump;

#endif
