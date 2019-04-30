#ifndef _XN297DUMP_PAGE_H_
#define _XN297DUMP_PAGE_H_

#define MAX_RF_CHANNEL 84
#define MAX_PAYLOAD 32

struct xn297dump_page {
    u8 last_packet[MAX_PAYLOAD];
};
#endif
