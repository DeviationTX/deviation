#ifndef _RFTOOLS_H_
#define _RFTOOLS_H_

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
    XN297DUMP_SCAN_OFF = 0,
    XN297DUMP_SCAN_ON,
    XN297DUMP_SCAN_SUCCESS,
};

enum {
    XN297DUMP_OFF = 0,
    XN297DUMP_1MBPS,
    XN297DUMP_250KBPS,
    XN297DUMP_2MBPS,
};

struct Xn297dump {
    u8 packet[32];
    u8 channel;
    u8 pkt_len;
    u8 crc_valid;
    u8 scan;
    u8 mode;
};

extern struct Xn297dump xn297dump;

#endif
