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

struct PageCfg {
    u8 trims;
    u8 barsize;
    u8 box[8];
    u8 bar[8];
    u8 toggle[NUM_TOGGLES];
    u8 tglico[NUM_TOGGLES][3];
    u8 quickpage[NUM_QUICKPAGES];
};

struct elem_xy {
    u16 x;
    u16 y;
};
struct elem_trim {
    struct elem_xy pos;
    u8 src;
    u8 is_vert;
};
struct elem_toggle {
    struct elem_xy pos;
    u8 src;
    u8 ico[3];
};
struct elem_box {
    struct elem_xy pos;
    u8 src;
    u8 type;
};
struct elem_modelico {
    struct elem_xy pos;
};
struct elem_bar {
    struct elem_xy pos;
    u8 src;
};

#define NUM_TRIM_ELEMS 6
#define NUM_BOX_ELEMS 8
#define NUM_BAR_ELEMS 8
#define NUM_TOGGLE_ELEMS 4

struct PageCfg2 {
    struct elem_modelico modelico;
    struct elem_trim     trim[NUM_TRIM_ELEMS];
    struct elem_toggle   tgl[NUM_TOGGLE_ELEMS];
    struct elem_box      box[NUM_BOX_ELEMS];
    struct elem_bar      bar[NUM_BOX_ELEMS];
};

#endif
