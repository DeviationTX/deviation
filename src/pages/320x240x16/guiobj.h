#ifndef _GUIOBJ_H_
#define _GUIOBJ_H_

#include "simple/simple.h"

struct dialog_obj {
    guiDialog_t dialog;
};

#define MAX_IDX (NUM_TX_BUTTONS > NUM_CHANNELS ? NUM_TX_BUTTONS : NUM_CHANNELS)
struct chantest_obj {
    guiLabel_t lock;
    guiLabel_t chan[NUM_CHANNELS];
    guiLabel_t value[MAX_IDX];
    guiBarGraph_t bar[MAX_IDX];
};

struct lang_obj {
    guiButton_t ok;
    guiListbox_t listbox;
};

struct maincfg_obj {
    guiScrollbar_t scrollbar;
    guiRect_t rect;
    union {
        struct {
            guiLabel_t trimlbl;
            guiTextSelect_t trim;
            guiLabel_t barlbl;
            guiTextSelect_t bar;
            guiLabel_t boxlbl[8];
            guiTextSelect_t box[8];
        } g1;
        struct {
            guiButton_t icon[4];
            guiTextSelect_t toggle[4];
            guiLabel_t barlbl[8];
            guiTextSelect_t bar[8];
        } g2;
        struct {
            guiLabel_t menulbl[4];
            guiTextSelect_t menu[4];
        } g3;
        struct {
            guiRect_t rect;
            guiImage_t image[40];
        } g4;
    } u;
};

struct mainpage_objs {
    guiButton_t optico;
    union {
        guiButton_t ico;
        guiImage_t img;
    } model;
    guiLabel_t name;
    guiBarGraph_t trim[6];
    guiLabel_t box[8];
    guiBarGraph_t bar[8];
    guiImage_t toggle[4];
    union {
        guiImage_t ico;
        guiLabel_t lbl;
    } batt;
    guiImage_t pwr;
};

struct modelcfg_objs {
    guiLabel_t title;
    guiLabel_t swashlbl;
    guiTextSelect_t swash;
    guiLabel_t invlbl[3];
    guiTextSelect_t inv[3];
    guiLabel_t mixlbl[3];
    guiTextSelect_t mix[3];

    guiLabel_t protolbl[NUM_PROTO_OPTS];
    guiTextSelect_t proto[NUM_PROTO_OPTS];
};

struct modelload_objs {
    guiButton_t ok;
    guiListbox_t list;
    guiImage_t image;
};

struct modelpage_objs {
    guiLabel_t filelbl;
    guiTextSelect_t file;
    guiLabel_t guilbl;
    guiTextSelect_t guits;
    guiLabel_t namelbl;
    guiButton_t name;
    guiButton_t icon;
    guiLabel_t typelbl;
    guiTextSelect_t type;
    guiLabel_t protolbl;
    guiTextSelect_t proto;
    guiLabel_t numchlbl;
    guiTextSelect_t numch;
    guiLabel_t pwrlbl;
    guiTextSelect_t pwr;
    guiLabel_t fixedidlbl;
    guiButton_t fixedid;
    guiButton_t bind;
    guiKeyboard_t keyboard;
};

struct page_obj {
    guiButton_t exitico;
    guiLabel_t title;
    guiButton_t previco;
    guiButton_t nextico;
};

struct reorder_obj {
    guiButton_t up;
    guiButton_t down;
    guiTextSelect_t value;
    guiButton_t apply;
    guiButton_t insert;
    guiButton_t remove;
    guiListbox_t list;
};

struct scanner_obj {
    guiButton_t enable;
    guiBarGraph_t bar[80];
};

struct telemcfg_obj {
    guiLabel_t msg;
    guiLabel_t name[TELEM_NUM_ALARMS];
    guiTextSelect_t type[TELEM_NUM_ALARMS];
    guiTextSelect_t gtlt[TELEM_NUM_ALARMS];
    guiTextSelect_t value[TELEM_NUM_ALARMS];
};

struct telemtest_obj {
   guiLabel_t msg;
   guiLabel_t templbl[4];
   guiLabel_t temp[4];
   guiLabel_t voltlbl[3];
   guiLabel_t volt[3];
   guiLabel_t rpmlbl[2];
   guiLabel_t rpm[2];
   guiLabel_t gpslbl[5];
   guiLabel_t gps[5];
};

struct timer_obj {
    guiLabel_t timer[NUM_TIMERS];
    guiTextSelect_t type[NUM_TIMERS];
    guiLabel_t switchlbl[NUM_TIMERS];
    guiTextSelect_t src[NUM_TIMERS];
    guiLabel_t startlbl[NUM_TIMERS];
    guiTextSelect_t start[NUM_TIMERS];
};

struct trim_obj {
    guiLabel_t inplbl;
    guiLabel_t neglbl;
    guiLabel_t poslbl;
    guiLabel_t steplbl;
    guiButton_t src[NUM_TRIMS];
    guiLabel_t neg[NUM_TRIMS];
    guiLabel_t pos[NUM_TRIMS];
    guiTextSelect_t step[NUM_TRIMS];
};

struct trimedit_obj {
    guiLabel_t srclbl;
    guiLabel_t neglbl;
    guiLabel_t poslbl;
    guiLabel_t steplbl;
    guiTextSelect_t src;
    guiTextSelect_t neg;
    guiTextSelect_t pos;
    guiTextSelect_t step;
};

struct tx_obj {
    guiScrollbar_t scrollbar;
    union {
        struct {
             guiLabel_t head1;
             guiLabel_t langlbl;
             guiButton_t lang;
             guiLabel_t modelbl;
             guiTextSelect_t mode;
             guiLabel_t touchlbl;
             guiButton_t touchcalib;
             guiButton_t touchtest;
             guiLabel_t battalrmlbl;
             guiTextSelect_t battalrm;
	     guiLabel_t battalrmintvllbl;
             guiTextSelect_t battalrmintvl;
             guiLabel_t sticklbl;
             guiButton_t stickcalib;
             guiLabel_t buzzlbl;
             guiTextSelect_t buzz;
        } g1;
        struct {
             guiLabel_t head1;
             guiLabel_t backlightlbl;
             guiTextSelect_t backlight;
             guiLabel_t dimtimelbl;
             guiTextSelect_t dimtime;
             guiLabel_t dimtgtlbl;
             guiTextSelect_t dimtgt;
             guiLabel_t head2;
             guiLabel_t templbl;
             guiTextSelect_t temp;
             guiLabel_t lengthlbl;
             guiTextSelect_t length;
        } g2;
        struct {
             guiLabel_t head1;
             guiLabel_t prealertlbl;
             guiTextSelect_t prealert;
             guiLabel_t preintvllbl;
             guiTextSelect_t preintvl;
             guiLabel_t timeuplbl;
             guiTextSelect_t timeup;
        } g3;
    } u;
};

struct calibrate_obj {
    guiLabel_t title;
    guiLabel_t msg;
    guiLabel_t msg1;
};

struct usb_obj {
    guiLabel_t msg;
};    

/****Advanced ****/
struct advcurve_obj {
    guiTextSelect_t name;
    guiLabel_t pointlbl;
    guiTextSelect_t point;
    guiLabel_t valuelbl;
    guiTextSelect_t value;
    guiXYGraph_t graph;
};

struct advlimit_obj {
    guiLabel_t title;
    guiLabel_t reverselbl;
    guiTextSelect_t reverse;
    guiLabel_t failsafelbl;
    guiTextSelect_t failsafe;
    guiLabel_t safetylbl;
    guiTextSelect_t safety;
    guiLabel_t safevallbl;
    guiTextSelect_t safeval;
    guiLabel_t minlbl;
    guiTextSelect_t min;
    guiLabel_t maxlbl;
    guiTextSelect_t max;
    guiLabel_t scalelbl;
    guiTextSelect_t scale;
    guiLabel_t scaleneglbl;
    guiTextSelect_t scaleneg;
    guiLabel_t subtrimlbl;
    guiTextSelect_t subtrim;
    guiLabel_t speedlbl;
    guiTextSelect_t speed;
};

struct advmixer_obj {
    guiButton_t testico;
    guiButton_t reorderico;
    union {
        guiButton_t but;
        guiLabel_t lbl;
    } name[ENTRIES_PER_PAGE];
    guiButton_t tmpl[ENTRIES_PER_PAGE];
    guiLabel_t src[ENTRIES_PER_PAGE];
    guiLabel_t sw1[ENTRIES_PER_PAGE];
    guiLabel_t sw2[ENTRIES_PER_PAGE];
    guiScrollbar_t scroll;
};

struct advmixcfg_obj {
    guiLabel_t chan;
    guiTextSelect_t tmpl;
    //guiBarGraph_t bar;
    //guiXYGraph_t graph;
    union {
        struct {
            guiLabel_t srclbl;
            guiTextSelect_t src;
            guiLabel_t curvelbl;
            guiTextSelect_t curve;
            guiXYGraph_t graph;
            guiLabel_t scalelbl;
            guiTextSelect_t scale;
            guiLabel_t offsetlbl;
            guiTextSelect_t offset;
        } g1;
        struct {
            guiLabel_t srclbl;
            guiLabel_t sw1lbl;
            guiLabel_t sw2lbl;
            guiTextSelect_t src;
            guiTextSelect_t sw1;
            guiTextSelect_t sw2;
            guiLabel_t high;
            guiButton_t rate[2];
            guiLabel_t linked[2];
            guiTextSelect_t curvehi;
            guiTextSelect_t curve[2];
            guiLabel_t scalelbl;
            guiTextSelect_t scalehi;
            guiTextSelect_t scale[2];
            guiXYGraph_t graphhi;
            guiXYGraph_t graph[2];
        } g2;
        struct {
            guiLabel_t nummixlbl;
            guiTextSelect_t nummix;
            guiLabel_t pagelbl;
            guiTextSelect_t page;
            guiLabel_t swlbl;
            guiTextSelect_t sw;
            guiLabel_t muxlbl;
            guiTextSelect_t mux;
            guiLabel_t srclbl;
            guiTextSelect_t src;
            guiLabel_t curvelbl;
            guiTextSelect_t curve;
            guiLabel_t scalelbl;
            guiTextSelect_t scale;
            guiLabel_t offsetlbl;
            guiTextSelect_t offset;
            guiXYGraph_t graph;
            guiBarGraph_t bar;
            guiButton_t trim;
        } g3;
    } u;
};

/******* Standard ********/
struct stdcurve_obj {
    guiLabel_t msg;
    guiLabel_t modelbl;
    guiTextSelect_t mode;
    guiTextSelect_t hold;
    guiButton_t auto_;
    guiLabel_t vallbl[9];
    guiTextSelect_t val[9];
    guiButton_t lock[7];
    guiXYGraph_t graph;
};

struct stddrexp_obj {
    guiLabel_t msg;
    guiLabel_t srclbl;
    guiTextSelect_t src;
    guiLabel_t mode[3];
    guiTextSelect_t dr[3];
    guiTextSelect_t exp[3];
    guiXYGraph_t graph[3];
};

struct stdchan_obj {
    guiLabel_t name[ENTRIES_PER_PAGE];
    guiTextSelect_t value[ENTRIES_PER_PAGE];
    guiScrollbar_t scrollbar;
};
struct stdgyro_obj {
    guiLabel_t msg;
    guiLabel_t chanlbl;
    guiTextSelect_t chan;
    guiLabel_t gyrolbl[3];
    guiTextSelect_t gyro[3];
};

struct stdmenu_obj {
    guiButton_t icon[20];
};

struct stdswash_obj {
    guiLabel_t typelbl;
    guiTextSelect_t type;
    guiLabel_t lbl[3];
    guiTextSelect_t mix[3];
};

struct stdswitch_obj {
    guiLabel_t modelbl;
    guiTextSelect_t mode;
    guiLabel_t tholdlbl;
    guiTextSelect_t thold;
    guiLabel_t gyrolbl;
    guiTextSelect_t gyro;
};

struct stdthold_obj {
    guiLabel_t enlbl;
    guiTextSelect_t en;
    guiLabel_t valuelbl;
    guiTextSelect_t value;
};

struct stdtravel_obj {
    guiLabel_t dnlbl;
    guiLabel_t uplbl;
    guiLabel_t name[ENTRIES_PER_PAGE];
    guiTextSelect_t down[ENTRIES_PER_PAGE];
    guiTextSelect_t up[ENTRIES_PER_PAGE];
    guiScrollbar_t scrollbar;
};

struct gui_objs {
    struct page_obj page;
    struct dialog_obj dialog;
    union {
        struct chantest_obj chantest;
        struct lang_obj lang;
        struct maincfg_obj maincfg;
        struct mainpage_objs mainpage;
        struct modelcfg_objs modelcfg;
        struct modelload_objs modelload;
        struct modelpage_objs modelpage;
        struct reorder_obj reorder;
#ifdef ENABLE_SCANNER
        struct scanner_obj scanner;
#endif
        struct telemcfg_obj telemcfg;
        struct telemtest_obj telemtest1;
        struct telemtest_obj telemtest2;
        struct timer_obj timer;
        struct trim_obj trim;
        struct trimedit_obj trimedit;
        struct tx_obj tx;
        struct calibrate_obj calibrate;
        struct usb_obj usb;

        struct advcurve_obj advcurve;
        struct advlimit_obj advlimit;
        struct advmixer_obj advmixer;
        struct advmixcfg_obj advmixcfg;

        struct stdcurve_obj stdcurve;
        struct stddrexp_obj stddrexp;
        struct stdgyro_obj stdgyro;
        struct stdmenu_obj stdmenu;
        struct stdswash_obj stdswash;
        struct stdswitch_obj stdswitch;
        struct stdthold_obj stdthold;
        struct stdtravel_obj stdtravel;
        struct stdchan_obj stdchan;
    } u;
} gui_objs;
#endif
