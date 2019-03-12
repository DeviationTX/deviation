#ifndef _SCANNER_PAGE_H_
#define _SCANNER_PAGE_H_

struct scanner_page {
    u8 enable;
    u8 bars_valid;
    enum Protocols model_protocol;
};
#endif
