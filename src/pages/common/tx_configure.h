#ifndef __TX_CONFIGURE_H_
#define __TX_CONFIGURE_H_
#include "config/tx.h"

struct tx_configure_page {
    u8 enable;
    u8 state;
    s8 selected;
    u8 total_items;
    struct touch coords;
    struct touch coords1;
    struct StickCalibration calibration[INP_HAS_CALIBRATION];
    void(*return_page)(int page);
    char tmpstr[30];
};
#endif
