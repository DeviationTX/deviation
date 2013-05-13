#ifndef _GUIOBJ_H_
#define _GUIOBJ_H_
struct about_obj {
    guiLabel_t label[3];
};

struct splash_obj {
    guiLabel_t version;
    guiImage_t splash_image;
};

struct chantest_obj {
    guiLabel_t title;
    guiRect_t  rect;
    guiLabel_t chan[8];
    guiLabel_t value[8];
    guiBarGraph_t bar[8];
    guiLabel_t page;
    guiScrollable_t scrollable;
};

struct dialog_obj {
    guiDialog_t dialog;
};

struct lang_obj {
    guiLabel_t label[4];
    guiScrollable_t scrollable;
};

#define ICONS_MAX_COUNT 30 
struct mainconfig_obj {
    guiLabel_t label[4];
    union {
        guiLabel_t label;
        guiButton_t button;
    } col1[4];
    guiTextSelect_t value[4];
    guiScrollable_t scrollable;
    guiImage_t image[ICONS_MAX_COUNT];
};

struct toggleselect_obj {
    guiButton_t  revert;
    guiLabel_t   switchbox;
    guiLabel_t   togglelabel[3];
    guiRect_t    separator;
    guiImage_t   toggleicon[3];
    guiImage_t   symbolicon[32];
    guiRect_t    symbolframe;
    guiScrollable_t scrollable;
};

struct mainpage_obj {
    guiLabel_t name;
    guiImage_t icon;
    guiBarGraph_t trim[6];
    guiBarGraph_t bar[8];
    guiLabel_t box[8];
    guiImage_t toggle[NUM_TOGGLES];
    guiLabel_t battery;
    guiLabel_t power;
};

struct menu_obj {
    guiLabel_t idx[4];
    guiLabel_t name[4];
    guiScrollable_t scrollable;
};

struct modelcfg_obj {
    guiLabel_t label[4];
    guiTextSelect_t value[4];
    guiScrollable_t scrollable;
};

struct modelload_obj {
    guiTextSelect_t ico;
    guiImage_t image;
    guiListbox_t listbox;
};

struct modelpage_obj {
    union {
        guiLabel_t label;
        guiTextSelect_t ts;
    } col1[4];
    union {
        guiTextSelect_t ts;
        guiButton_t but;
    } col2[4];
    guiLabel_t file;
    guiScrollable_t scrollable;
    guiKeyboard_t  keyboard;
};

struct reorder_obj {
    guiButton_t up;
    guiButton_t down;
    guiTextSelect_t value;
    guiButton_t apply;
    guiButton_t insert;
    guiButton_t remove;
    guiButton_t save;
    guiListbox_t list;
};

struct telemcfg_obj {
    guiLabel_t msg;
    guiLabel_t idx[4];
    guiTextSelect_t name[4];
    guiTextSelect_t gtlt[4];
    guiTextSelect_t value[4];
    guiScrollable_t scrollable;
};

struct telemtest1_obj {
    guiLabel_t page;
    guiLabel_t msg;
    guiLabel_t tempstr;
    guiLabel_t voltstr;
    guiLabel_t rpmstr;
    guiLabel_t idx[4];
    guiLabel_t temp[4];
    guiLabel_t volt[3];
    guiLabel_t rpm[2];
};

struct telemtest2_obj {
    guiLabel_t page;
    guiLabel_t gpsstr[2];
    guiLabel_t gps[2];
    guiScrollable_t scrollable;
};

struct timer_obj {
    guiLabel_t name;
    guiTextSelect_t type;
    guiLabel_t switchlbl;
    guiTextSelect_t src;
    guiLabel_t startlbl;
    guiTextSelect_t start;
    guiLabel_t resetlbl;
    guiTextSelect_t resetsrc;
    guiLabel_t resetpermlbl;
    guiButton_t resetperm;
    guiScrollable_t scrollable;
};

struct trim_obj {
    guiLabel_t steplbl;
    guiLabel_t trimposlbl;
    guiButton_t src[4];
    guiTextSelect_t item[4];
    guiLabel_t name[4];
    guiScrollable_t scrollable;
};
struct trim2_obj {
    guiButton_t save;
    guiLabel_t label[4];
    guiTextSelect_t value[4];
    guiScrollable_t scrollable;
};

struct tx_obj {
    guiLabel_t title;
    guiLabel_t label[4];
    union {
        guiButton_t but;
        guiTextSelect_t ts;
    } value[4];
    guiScrollable_t scrollable;
};

struct calibrate_obj {
    guiLabel_t title;
    guiLabel_t msg;
    guiLabel_t msg1;
};
    
struct usb_obj {
    guiLabel_t label;
};

/****Advanced ****/
struct advcurve_obj {
    guiTextSelect_t name;
    guiButton_t save;
    guiRect_t rect;
    guiLabel_t pointlbl;
    guiTextSelect_t point;
    guiLabel_t valuelbl;
    guiTextSelect_t value;
    guiLabel_t smoothlbl;
    guiTextSelect_t smooth;
    guiXYGraph_t graph;
};

struct advlimit_obj {
    guiLabel_t title;
    guiButton_t revert;
    guiScrollable_t scrollable;
    guiLabel_t label[4];
    guiTextSelect_t value[4];
};

struct advmixer_obj {
    guiButton_t limit[2];
    guiLabel_t  name[2];
    guiButton_t tmpl[2];
    guiLabel_t src[2];
    guiLabel_t sw1[2];
    guiLabel_t sw2[2];
    guiScrollbar_t scroll;
};

struct advmixcfg_obj {
    guiLabel_t chan;
    guiTextSelect_t tmpl;
    guiButton_t save;
    guiRect_t rect1;
    guiBarGraph_t bar;
    guiXYGraph_t graph;
    guiLabel_t label[4];
    union {
        guiButton_t but;
        guiTextSelect_t ts;
    } value[4];
    guiScrollable_t scrollable;
};

/******* Standard ********/
struct stdcurve_obj {
    guiLabel_t msg;
    guiTextSelect_t mode;
    guiTextSelect_t hold;
    guiButton_t auto_;
    guiLabel_t vallbl[9];
    guiTextSelect_t val[9];
    guiXYGraph_t graph;
};

struct stddrexp_obj {
    union {
        guiLabel_t msg;
        guiTextSelect_t type;
    } u;
    guiLabel_t label[2];
    guiTextSelect_t value1[2];
    guiTextSelect_t value2[2];
    guiXYGraph_t graph;
    guiScrollable_t scrollable;
    
};

struct stdchan_obj {
    guiLabel_t title;
    guiLabel_t name[4];
    guiTextSelect_t value[4];
    guiScrollable_t scrollable;
};
struct stdgyro_obj {
    guiLabel_t msg;
    guiLabel_t chanlbl;
    guiTextSelect_t chan;
    guiLabel_t gyrolbl[3];
    guiTextSelect_t gyro[3];
};

struct stdswash_obj {
    guiTextSelect_t type;
    guiLabel_t lbl[3];
    guiTextSelect_t mix[3];
};

struct stdswitch_obj {
    guiLabel_t name[4];
    guiTextSelect_t value[4];
    guiScrollable_t scrollable;
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
    guiLabel_t chan[4];
    guiTextSelect_t dn[4];
    guiTextSelect_t up[4];
    guiScrollable_t scrollable;
};

struct gui_objs {
    struct dialog_obj dialog;
    union {
        struct about_obj about;
        struct splash_obj splash;
        struct chantest_obj chantest;
        struct lang_obj lang;
        struct mainconfig_obj mainconfig;
        struct mainpage_obj mainpage;
        struct menu_obj menu;
        struct modelcfg_obj modelcfg;
        struct modelload_obj modelload;
        struct modelpage_obj modelpage;
        struct reorder_obj reorder;
        struct telemcfg_obj telemcfg;
        struct telemtest1_obj telemtest1;
        struct telemtest2_obj telemtest2;
        struct timer_obj timer;
        struct trim_obj trim;
        struct trim2_obj trim2;
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
        struct stdswash_obj stdswash;
        struct stdswitch_obj stdswitch;
        struct stdthold_obj stdthold;
        struct stdtravel_obj stdtravel;
        struct stdchan_obj stdchan;
        struct toggleselect_obj toggleselect;
    } u;
} gui_objs;

#endif
