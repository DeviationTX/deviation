#ifndef _SCANNER_PAGE_H_
#define _SCANNER_PAGE_H_

struct scanner_page {
    u8 rssi[255];
    u8 channel;
    u8 chan_min;
    u8 chan_max;
    u8 scanState;
    u8 time_to_scan;
    u8 enable;
    u8 scan_mode;
    u8 attenuator;
    u8 averaging;
    u8 bars_valid;
};
#endif
