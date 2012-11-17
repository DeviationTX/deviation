#ifndef _PAGECFG_H_
#define _PAGECFG_H_

#define NUM_QUICKPAGES 4
enum DisplayTrims {
    TRIMS_NONE,
    TRIMS_4OUTSIDE,
    TRIMS_4INSIDE,
    TRIMS_6,
    TRIMS_LAST,
};

enum BarSize {
    BARS_NONE,
    BARS_4,
    BARS_8,
    BARS_LAST,
};

struct PageCfg {
    u8 trims;
    u8 barsize;
    u8 box[8];
    u8 bar[8];
    u8 toggle[4];
    u8 tglico[4];
    u8 quickpage[NUM_QUICKPAGES];
    u8 quickbtn[2];
};
#endif
