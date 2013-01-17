#ifndef _SCANNER_PAGE_H_
#define _SCANNER_PAGE_H_

#define MIN_RADIOCHANNEL     0x04
#define MAX_RADIOCHANNEL     0x54

struct scanner_page {
    u8 channelnoise[MAX_RADIOCHANNEL - MIN_RADIOCHANNEL];
    u8 channel;
    u8 time_to_scan;
    u8 enable;
};
#endif
