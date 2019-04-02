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

struct Xn297dump {
    u8 packet[32];
    u8 channel;
    u8 pkt_len;
//    u8 crc_valid;
};

extern struct Xn297dump xn297dump;

#endif
