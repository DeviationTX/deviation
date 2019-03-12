#ifndef _RFTOOLS_H_
#define _RFTOOLS_H_

#if SUPPORT_SCANNER

struct Scanner {
    u8 rssi[255];
    u8 chan_min;
    u8 chan_max;
    u8 attenuator;
    u8 averaging;
};

extern struct Scanner Scanner;

#endif  // SUPPORT_SCANNER
#endif
