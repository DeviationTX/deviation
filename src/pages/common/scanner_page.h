#ifndef _SCANNER_PAGE_H_
#define _SCANNER_PAGE_H_

enum ScannerMode {
    PEAK_MODE,
    AVERAGE_MODE,
    PEAK_HOLD_AVERAGE_MODE,
    LAST_MODE,
};

struct scanner_page {
    u8 enable;
    u8 bars_valid;
    enum ScannerMode mode;
    enum Protocols model_protocol;
};
#endif
