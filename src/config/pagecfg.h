#ifndef _PAGECFG_H_
#define _PAGECFG_H_
#define GUI_ADVANCED 1
#include "gui/gui.h"

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

struct elem {
    u16 x;
    u16 y;
    u8 src;
    u8 type;
    union {
        // for vert
        // u8 is_vert;
        // for toggle
        u8 ico[3];
    }extra;
};

//NUM_TRIM_ELEMS + NUM_BOX_ELEMS + NUM_BAR_ELEMS + NUM_TOGGLE_ELEMS
#ifndef NUM_ELEMS
    #define NUM_ELEMS (6 + 8 + 8 + 4 + 1)
#endif
//#define NUM_TRIM_ELEMS 6
//#define NUM_BOX_ELEMS 8
//#define NUM_BAR_ELEMS 8
//#define NUM_TOGGLE_ELEMS 4


enum {
    ELEM_NONE,
    ELEM_SMALLBOX,
    ELEM_BIGBOX,
    ELEM_TOGGLE,
    ELEM_BAR,
    ELEM_VTRIM,
    ELEM_HTRIM,
    ELEM_MODELICO,
    ELEM_BATTERY,
    ELEM_TXPOWER,
    ELEM_LAST,
};

#endif
