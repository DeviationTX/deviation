#ifndef _GUIOBJ_H_
#define _GUIOBJ_H_
struct about_obj {
    guiLabel_t label[3];
};

struct chantest_obj {
    guiLabel_t title;
    guiRect_t  rect;
    guiLabel_t chan[NUM_TEST_BARS];
    guiLabel_t value[NUM_VALUES];
    guiBarGraph_t bar[NUM_TEST_BARS];
    guiLabel_t page;
    guiScrollbar_t scroll;
    guiLabel_t lock;
};

struct dialog_obj {
    guiDialog_t dialog;
};

struct lang_obj {
    guiLabel_t label[20]; /*FIXME */
    guiScrollbar_t scroll;
};

struct mainconfig_obj {
    guiRect_t rect;
    guiLabel_t label_trim;
    guiTextSelect_t trimsel;
    guiLabel_t boxlabel[8];
    guiTextSelect_t boxsel[8];
    guiLabel_t switchlabel[4];
    guiTextSelect_t switchsel[4];
    guiLabel_t menulabel[NUM_QUICKPAGES];
    guiTextSelect_t menusel[NUM_QUICKPAGES];
    guiScrollbar_t scroll_bar;
};

struct mainpage_obj {
    guiLabel_t name;
    guiImage_t icon;
    guiBarGraph_t trim[6];
    guiBarGraph_t bar[8];
    guiLabel_t box[8];
    guiLabel_t toggle[4];
    guiLabel_t battery;
    guiLabel_t power;
};

struct menu_obj {
    guiLabel_t idx[20];  /* FIXME */
    guiLabel_t name[20]; /* FIXME */
    guiScrollbar_t scroll;
};

struct modelcfg_obj {
    guiLabel_t swash;
    guiTextSelect_t swashsel;
    guiLabel_t ele;
    guiTextSelect_t elesel;
    guiLabel_t ail;
    guiTextSelect_t ailsel;
    guiLabel_t col;
    guiTextSelect_t colsel;
    guiLabel_t emix;
    guiTextSelect_t emixsel;
    guiLabel_t amix;
    guiTextSelect_t amixsel;
    guiLabel_t cmix;
    guiTextSelect_t cmixsel;
    guiScrollbar_t scroll;
};

struct modelproto_obj {
    guiLabel_t str[NUM_PROTO_OPTS];
    guiTextSelect_t sel[NUM_PROTO_OPTS];
};
 
struct modelload_obj {
    guiTextSelect_t ico;
    guiImage_t image;
    guiListbox_t listbox;
};

struct modelpage_obj {
    guiLabel_t file;
    guiTextSelect_t filesel;
    guiLabel_t name;
    guiButton_t namesel;
    guiLabel_t icon;
    guiButton_t iconsel;
    guiLabel_t type;
    guiTextSelect_t typesel;
    guiLabel_t power;
    guiTextSelect_t powersel;
    guiTextSelect_t protosel;
    guiButton_t bind;
    guiLabel_t id;
    guiButton_t idsel;
    guiLabel_t chan;
    guiTextSelect_t chansel;
    guiLabel_t mixer;
    guiTextSelect_t mixersel;
    guiScrollbar_t scroll;
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
    guiLabel_t idx[TELEM_NUM_ALARMS];
    guiTextSelect_t name[TELEM_NUM_ALARMS];
    guiTextSelect_t gtlt[TELEM_NUM_ALARMS];
    guiTextSelect_t value[TELEM_NUM_ALARMS];
    guiScrollbar_t scroll;
};

struct telemtest_obj {
    guiLabel_t msg;
    guiLabel_t tempstr;
    guiLabel_t voltstr;
    guiLabel_t rpmstr;
    guiLabel_t page;
    guiLabel_t idx[4];
    guiLabel_t temp[4];
    guiLabel_t volt[3];
    guiLabel_t rpm[2];
    guiLabel_t gpsstr[5];
    guiLabel_t gps[5];
    guiScrollbar_t scroll;
};

struct timer_obj {
    guiLabel_t name[8];
    guiTextSelect_t type[8];
    guiLabel_t switchlbl[8];
    guiTextSelect_t src[8];
    guiLabel_t startlbl[8];
    guiTextSelect_t start[8];
    guiScrollbar_t scroll;
};

struct trim_obj {
    guiLabel_t steplbl;
    guiLabel_t trimposlbl;
    guiButton_t src[NUM_TRIMS];
    guiTextSelect_t item[NUM_TRIMS];
    guiLabel_t name[NUM_TRIMS];
    guiScrollbar_t scroll;
};
struct trim2_obj {
    guiButton_t save;
    guiLabel_t srclbl;
    guiTextSelect_t src;
    guiLabel_t trimneglbl;
    guiTextSelect_t trimneg;
    guiLabel_t trimposlbl;
    guiTextSelect_t trimpos;
    guiLabel_t steplbl;
    guiTextSelect_t step;
};

struct tx_obj {
    guiLabel_t lbl1;
    guiLabel_t langlbl;
    guiButton_t lang;
    guiLabel_t modelbl;
    guiTextSelect_t mode;
    guiLabel_t batlbl;
    guiTextSelect_t bat;
    guiLabel_t sticklbl;
    guiButton_t stick;
    guiLabel_t buzzlbl;
    guiTextSelect_t buzz;
    guiLabel_t hapticlbl;
    guiTextSelect_t haptic;

    guiLabel_t lbl2;
    guiLabel_t lightlbl;
    guiTextSelect_t light;
    guiLabel_t contrastlbl;
    guiTextSelect_t contrast;
    guiLabel_t dimlbl;
    guiTextSelect_t dim;
    guiLabel_t dimtgtlbl;
    guiTextSelect_t dimtgt;
  
    guiLabel_t lbl3;
    guiLabel_t prealertlbl;
    guiTextSelect_t prealert;
    guiLabel_t prevallbl;
    guiTextSelect_t preval;
    guiLabel_t timeuplbl;
    guiTextSelect_t timeup;

    guiLabel_t lbl4;
    guiLabel_t templbl;
    guiTextSelect_t temp;
    guiLabel_t lenlbl;
    guiTextSelect_t len;

    guiScrollbar_t scroll;
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
    guiXYGraph_t graph;
};

struct advlimit_obj {
    guiLabel_t title;
    guiButton_t revert;
    guiLabel_t reverselbl;
    guiTextSelect_t reverse;
    guiLabel_t failsafelbl;
    guiTextSelect_t failsafe;
    guiLabel_t safelbl;
    guiTextSelect_t safe;
    guiLabel_t safevallbl;
    guiTextSelect_t safeval;
    guiLabel_t minlbl;
    guiTextSelect_t min;
    guiLabel_t maxlbl;
    guiTextSelect_t max;
    guiLabel_t scaleposlbl;
    guiTextSelect_t scalepos;
    guiLabel_t scaleneglbl;
    guiTextSelect_t scaleneg;
    guiLabel_t subtrimlbl;
    guiTextSelect_t subtrim;
    guiLabel_t speedlbl;
    guiTextSelect_t speed;
    guiScrollbar_t scroll;
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
    guiLabel_t srclbl;
    guiTextSelect_t src;
    guiLabel_t curvelbl;
    guiTextSelect_t curve;
    guiLabel_t scalelbl;
    guiTextSelect_t scale;
    guiLabel_t offsetlbl;
    guiTextSelect_t offset;
    guiLabel_t swlbl;
    guiTextSelect_t sw;
    guiLabel_t nummixlbl;
    guiTextSelect_t nummix;
    guiLabel_t pagelbl;
    guiTextSelect_t page;
    guiLabel_t muxlbl;
    guiTextSelect_t mux;
    guiButton_t trim;
    guiLabel_t highlbl;
    guiTextSelect_t high;
    guiLabel_t sw1lbl;
    guiTextSelect_t sw1;
    guiLabel_t sw2lbl;
    guiTextSelect_t sw2;
    guiLabel_t scale1lbl;
    guiLabel_t scale2lbl;
    guiRect_t rect1;
    guiRect_t rect2;
    guiButton_t link[2];
    guiTextSelect_t curve_[2];
    guiTextSelect_t scale_[2];
    guiXYGraph_t graphs[3];
    guiBarGraph_t bar;
    guiScrollbar_t scroll;
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
    guiLabel_t msg;
    guiTextSelect_t type;
    guiLabel_t modelbl[3];
    guiTextSelect_t dr[3];
    guiTextSelect_t exp[3];
    guiXYGraph_t graphs[3];
    guiScrollbar_t scroll;
};

struct stdfailsafe_obj {
    guiLabel_t name[NUM_CHANNELS];
    guiTextSelect_t failsafe[NUM_CHANNELS];
    guiScrollbar_t scroll;
};

struct stdgyro_obj {
    guiLabel_t msg;
    guiLabel_t chanlbl;
    guiTextSelect_t chan;
    guiLabel_t gyrolbl[3];
    guiTextSelect_t gyro[3];
};

struct stdreverse_obj {
    guiLabel_t name[NUM_CHANNELS];
    guiTextSelect_t reverse[NUM_CHANNELS];
    guiScrollbar_t scroll;
};

struct stdsubtrim_obj {
    guiLabel_t name[NUM_CHANNELS];
    guiTextSelect_t subtrim[NUM_CHANNELS];
    guiScrollbar_t scroll;
};

struct stdswash_obj {
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
    guiLabel_t chan[NUM_CHANNELS];
    guiTextSelect_t dn[NUM_CHANNELS];
    guiTextSelect_t up[NUM_CHANNELS];
    guiScrollbar_t scroll;
};

struct gui_objs {
    struct dialog_obj dialog;
    union {
        struct about_obj about;
        struct chantest_obj chantest;
        struct lang_obj lang;
        struct mainconfig_obj mainconfig;
        struct mainpage_obj mainpage;
        struct menu_obj menu;
        struct modelcfg_obj modelcfg;
        struct modelproto_obj modelproto;
        struct modelload_obj modelload;
        struct modelpage_obj modelpage;
        struct reorder_obj reorder;
        struct telemcfg_obj telemcfg;
        struct telemtest_obj telemtest;
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
        struct stdfailsafe_obj stdfailsafe;
        struct stdgyro_obj stdgyro;
        struct stdreverse_obj stdreverse;
        struct stdsubtrim_obj stdsubtrim;
        struct stdswash_obj stdswash;
        struct stdswitch_obj stdswitch;
        struct stdthold_obj stdthold;
        struct stdtravel_obj stdtravel;
    } u;
} gui_objs;

#endif
