#ifndef _GUIOBJ_H_
#define _GUIOBJ_H_

#include "standard/standard.h"

struct splash_obj {
    guiLabel_t version;
    guiImage_t splash_image;
};

struct dialog_obj {
    guiDialog_t dialog;
};

#define SEPARATION        32
#define NUM_BARS_PER_ROW  (LCD_WIDTH == 320 ? 8 : 14)
#define MAX_CHAN_ELEMENTS ((NUM_BARS_PER_ROW + 1) * 2) // 320: 2 rows with 9 elements each, 480: 2*15 resp. 2*8 / 2*14 with scrollbar
#define MAX_BUTT          (NUM_TX_BUTTONS > NUM_CHANNELS ? NUM_TX_BUTTONS : NUM_CHANNELS)
#define MAX_CHAN          (MAX_CHAN_ELEMENTS > MAX_BUTT ? MAX_CHAN_ELEMENTS : MAX_BUTT)
#define MAX_IDX           (MAX_CHAN > MAX_BUTT ? MAX_CHAN : MAX_BUTT)
#define TIMERS_PER_PAGE   (LCD_WIDTH == 320 ? 2 : 4)
#define LISTBOX_ITEMS     ((LCD_HEIGHT - 48) / 24)

struct chantest_obj {
    guiLabel_t lock;
    guiLabel_t chan[MAX_CHAN];
    guiLabel_t value[MAX_IDX];
    guiBarGraph_t bar[MAX_IDX];
    guiScrollable_t scrollable;
};

struct range_obj {
    guiLabel_t info;
    guiButton_t button;
};

struct lang_obj {
    guiButton_t ok;
    guiLabel_t label[LISTBOX_ITEMS];
    guiScrollable_t scrollable;
};

#define NUM_SYMBOL_COLS ((LCD_WIDTH-24) / 40)
#define NUM_SYMBOL_ROWS ((LCD_HEIGHT-(80-(40-31))) / 40)    //allows one row more to show (no gap for last row needed)
#define NUM_SYMBOL_ELEMS (NUM_SYMBOL_COLS * NUM_SYMBOL_ROWS)
struct toggle_obj {
    guiButton_t  revert;
    guiLabel_t   switchbox;
    guiLabel_t   togglelabel[3];
    guiRect_t    toggleframe;
    guiImage_t   toggleicon[3];
    guiImage_t   symbolicon[NUM_SYMBOL_ELEMS];
    guiRect_t    symbolframe;
    guiScrollbar_t scrollbar;
};

#define LAYDLG_Y_SPACE 10
#define LAYDLG_HEIGHT (LCD_HEIGHT - 32 - 2*LAYDLG_Y_SPACE)
#define LAYDLG_SCROLLABLE_Y (LCD_HEIGHT == 240 ? 36 : 32)
#define LAYDLG_TEXT_HEIGHT 20
#define LAYDLG_SCROLLABLE_HEIGHT (LAYDLG_HEIGHT - 2 * LAYDLG_SCROLLABLE_Y)
#define LAYDLG_NUM_ITEMS (LAYDLG_SCROLLABLE_HEIGHT / LAYDLG_TEXT_HEIGHT + 1)

struct mainlayout_obj {
    struct LabelDesc desc[5];
    //guiTextSelect_t newelem;
    guiButton_t newelem;
    guiButton_t editelem;
    guiTextSelect_t x;
    guiTextSelect_t y;
    guiLabel_t xlbl;
    guiLabel_t ylbl;
    //dialog
    guiDialog_t dialog;
    guiScrollable_t scrollable;
    guiLabel_t dlglbl[LAYDLG_NUM_ITEMS];
    guiTextSelect_t dlgts[LAYDLG_NUM_ITEMS];
    guiButton_t dlgbut[LAYDLG_NUM_ITEMS];
    guiButton_t dlgbut2[LAYDLG_NUM_ITEMS];
    //Everything below here must be part an element
    guiLabel_t elem[NUM_ELEMS];
};

struct mainpage_obj {
    guiButton_t optico;
    guiButton_t modelico;
    guiLabel_t name;
    union {
        guiBarGraph_t bar;
        guiLabel_t    box;
        guiImage_t    img;
    } elem[NUM_ELEMS];
    union {
        guiImage_t ico;
        guiLabel_t lbl;
    } batt;
#if HAS_RTC
    guiLabel_t time;
#endif
    guiImage_t pwr;
};

struct menu_obj {
    guiLabel_t idx[ENTRIES_BY_SCREENSIZE];
    guiLabel_t name[ENTRIES_BY_SCREENSIZE];
    guiScrollable_t scrollable;
};

#define DATALOG_NUM_SCROLLABLE (LCD_HEIGHT == 240 ? 7 : 8)
struct datalog_obj {
    guiLabel_t enlbl;
    guiTextSelect_t en;
    guiLabel_t freqlbl;
    guiTextSelect_t freq;
    guiButton_t reset;
    guiLabel_t remaining;
    guiLabel_t selectlbl;
    guiButton_t all;
    guiButton_t none;
    guiLabel_t checked[DATALOG_NUM_SCROLLABLE];
    guiLabel_t source[DATALOG_NUM_SCROLLABLE];
    guiLabel_t checked2[DATALOG_NUM_SCROLLABLE];
    guiLabel_t source2[DATALOG_NUM_SCROLLABLE];
    guiScrollable_t scrollable;
};

struct modelcfg_obj {
    guiLabel_t title;
    guiLabel_t swashlbl;
    guiTextSelect_t swash;
    guiLabel_t invlbl[3];
    guiTextSelect_t inv[3];
    guiLabel_t mixlbl[3];
    guiTextSelect_t mix[3];

    guiLabel_t protolbl[NUM_PROTO_OPTS];
    guiTextSelect_t proto[NUM_PROTO_OPTS];

    guiLabel_t numchlbl;
    guiTextSelect_t numch;
    guiLabel_t trainswlbl;
    guiTextSelect_t trainsw;
    guiLabel_t centerpwlbl;
    guiTextSelect_t centerpw;
    guiLabel_t deltapwlbl;
    guiTextSelect_t deltapw;
    guiLabel_t ppmmaplbl[MAX_PPM_IN_CHANNELS];
    guiTextSelect_t ppmmap[MAX_PPM_IN_CHANNELS];
};

struct modelload_obj {
    guiButton_t ok;
    guiImage_t image;
    guiLabel_t label[LISTBOX_ITEMS];
    guiScrollable_t scrollable;
};

struct modelpage_obj {
    guiLabel_t filelbl;
    guiTextSelect_t file;
    guiLabel_t guilbl;
    guiTextSelect_t guits;
    guiLabel_t namelbl;
    guiButton_t name;
    guiButton_t icon;
    guiLabel_t typelbl;
    guiTextSelect_t type;
    guiLabel_t ppmlbl;
    guiTextSelect_t ppm;
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
    guiTextSelect_t copy;
    guiButton_t apply;
    guiButton_t insert;
    guiButton_t remove;
    guiButton_t save;
    guiLabel_t  name[LISTBOX_ITEMS];
    guiScrollable_t scrollable;
};

struct scanner_obj {
    guiButton_t enable;
    guiTextSelect_t averaging;
    guiTextSelect_t attenuator;
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
   guiLabel_t label[30];
   guiLabel_t value[30];
};

struct timer_obj {
    guiScrollable_t scrollable;
    guiLabel_t timer[TIMERS_PER_PAGE];
    guiTextSelect_t type[TIMERS_PER_PAGE];
    guiLabel_t switchlbl[TIMERS_PER_PAGE];
    guiTextSelect_t src[TIMERS_PER_PAGE];
    guiLabel_t resetlbl[TIMERS_PER_PAGE];
    guiTextSelect_t resetsrc[TIMERS_PER_PAGE];
    guiLabel_t startlbl[TIMERS_PER_PAGE];
    guiLabel_t resetpermlbl[TIMERS_PER_PAGE];
    guiButton_t resetperm[TIMERS_PER_PAGE];
    guiButton_t setperm[TIMERS_PER_PAGE];
    guiLabel_t timelbl[TIMERS_PER_PAGE];
    guiLabel_t timevallbl[TIMERS_PER_PAGE];
    guiLabel_t datelbl[TIMERS_PER_PAGE];
    guiLabel_t datevallbl[TIMERS_PER_PAGE];
    guiLabel_t setlbl[TIMERS_PER_PAGE];
    guiButton_t set[TIMERS_PER_PAGE];
    guiTextSelect_t start[TIMERS_PER_PAGE];
};

struct settimer_obj {
    guiLabel_t      headerlbl;
    guiLabel_t      secondlbl, minutelbl, hourlbl;
    guiTextSelect_t secondsel, minutesel, hoursel;
    guiLabel_t      oldlbl, addlbl, newlbl;
    guiLabel_t      oldvalue, addvalue, newvalue;
    guiButton_t     addbtn, setbtn;
};

#define NUM_TRIM_ROWS (LCD_HEIGHT == 240 ? 7 : 8)
struct trim_obj {
    guiLabel_t inplbl;
    guiLabel_t neglbl;
    guiLabel_t poslbl;
    guiLabel_t steplbl;
    guiButton_t src[NUM_TRIM_ROWS];
    guiLabel_t neg[NUM_TRIM_ROWS];
    guiLabel_t pos[NUM_TRIM_ROWS];
    guiTextSelect_t step[NUM_TRIM_ROWS];
    guiScrollable_t scrollable;
};

struct trimedit_obj {
    guiLabel_t srclbl;
    guiLabel_t neglbl;
    guiLabel_t poslbl;
    guiLabel_t steplbl;
    guiLabel_t swlbl;
    guiTextSelect_t src;
    guiTextSelect_t neg;
    guiTextSelect_t pos;
    guiTextSelect_t step;
    guiTextSelect_t sw;
};

struct tx_obj {
    guiScrollable_t scrollable;
    union {
#if LCD_WIDTH != 480
        struct tx_obj_g1
#else
        struct tx_obj_g3
#endif
        {
             guiLabel_t head1_1;
             guiLabel_t langlbl;
             guiButton_t lang;
             guiLabel_t modelbl;
             guiTextSelect_t mode;
             guiLabel_t touchlbl;
             guiButton_t touchcalib;
             guiButton_t touchtest;
             guiLabel_t sticklbl;
             guiButton_t stickcalib;
#if HAS_RTC
             guiLabel_t clocklbl;
             guiButton_t clock;
#endif
#if LCD_WIDTH != 480
        } g1;
        struct tx_obj_g2 {
#endif
             guiLabel_t head2_1;
             guiLabel_t buzzlbl;
             guiTextSelect_t buzz;
             guiLabel_t audiolbl;
             guiTextSelect_t audio;
             guiTextSelect_t power_alarm;
             guiLabel_t power_alarmlbl;
             guiLabel_t battalrmlbl;
             guiTextSelect_t battalrm;
             guiLabel_t battalrmintvllbl;
             guiTextSelect_t battalrmintvl;
             guiLabel_t musicshutdbl;
             guiTextSelect_t music_shutdown;
             guiLabel_t head2_2;
             guiLabel_t backlightlbl;
             guiTextSelect_t backlight;
             guiLabel_t dimtimelbl;
             guiTextSelect_t dimtime;
             guiLabel_t dimtgtlbl;
             guiTextSelect_t dimtgt;
#if LCD_WIDTH != 480
        } g2;
        struct tx_obj_g3 {
#endif
             guiLabel_t head3_1;
             guiLabel_t prealertlbl;
             guiTextSelect_t prealert;
             guiLabel_t preintvllbl;
             guiTextSelect_t preintvl;
             guiLabel_t timeuplbl;
             guiTextSelect_t timeup;
             guiLabel_t head3_2;
             guiLabel_t templbl;
             guiTextSelect_t temp;
             guiLabel_t lengthlbl;
             guiTextSelect_t length;
             guiLabel_t telemivllbl;
             guiTextSelect_t telemivl;
             //These are for the virating motor which may or may-not be installed
             guiLabel_t head3_3;
             guiLabel_t viblbl;
             guiTextSelect_t vib;

        } g3;
    } u;
};

struct calibrate_obj {
    guiLabel_t title;
    guiLabel_t msg;
    guiLabel_t msg1;
};

struct usb_obj {
    guiLabel_t headline;
    guiLabel_t msg;
    guiLabel_t time;
    guiLabel_t date;
};

struct rtc_obj {
    guiLabel_t      title;
    // time / date format
    guiLabel_t      timelbl;
    guiLabel_t      datelbl;
    guiTextSelect_t timeformat;
    guiTextSelect_t dateformat;
    // array for second ... year
    guiLabel_t      label[6];
    guiTextSelect_t select[6];
    // label for displaying time / date
    guiLabel_t      actlbl;
    guiLabel_t      acttime;
    guiLabel_t      actdate;
    guiLabel_t      newlbl;
    guiLabel_t      newtime;
    guiLabel_t      newdate;
    // buttons for setting time and date
    guiButton_t     settime;
    guiButton_t     setdate;
};

#define DEBUG_LINE_COUNT (LCD_WIDTH == 320 ? 10 : 12)
struct debuglog_obj {
    guiLabel_t      line[DEBUG_LINE_COUNT];
    guiScrollable_t scrollable;
};

#ifdef HAS_MUSIC_CONFIG
struct voiceconfig_obj {
    guiLabel_t msg;
    guiScrollable_t scrollable;
    guiLabel_t name[10];
    guiTextSelect_t voiceidx[10];
    guiLabel_t voicelbl[10];
};
#endif

/****Advanced ****/
struct advcurve_obj {
    guiTextSelect_t name;
    guiLabel_t pointlbl;
    guiTextSelect_t point;
    guiLabel_t valuelbl;
    guiTextSelect_t value;
    guiTextSelect_t smooth;
    guiLabel_t smoothlbl;
    guiXYGraph_t graph;
};

struct advlimit_obj {
    guiButton_t revert;
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
    guiKeyboard_t keyboard;
    guiScrollable_t scrollable;
};

struct advmixcfg_obj {
    guiLabel_t chan;
    guiTextSelect_t tmpl;
    //guiBarGraph_t bar;
    //guiXYGraph_t graph;
    union {
        struct advmixcfg_obj_g1 {
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
        struct advmixcfg_obj_g2 {
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
        struct advmixcfg_obj_g3 {
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
    guiLabel_t holdlbl;
    guiLabel_t holdsw;
    guiTextSelect_t mode;
    guiTextSelect_t hold;
    guiButton_t auto_;
    guiLabel_t vallbl[9];
    guiTextSelect_t val[9];
    guiButton_t lock[9];
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
    guiLabel_t rangelbl;
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
    guiLabel_t draillbl;
    guiTextSelect_t drail;
    guiLabel_t drelelbl;
    guiTextSelect_t drele;
    guiLabel_t drrudlbl;
    guiTextSelect_t drrud;
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

struct _gui_objs {
    struct page_obj page;
    struct dialog_obj dialog;
    union {
        struct splash_obj splash;
        struct chantest_obj chantest;
        struct range_obj range;
        struct lang_obj lang;
        struct toggle_obj toggle;
        struct mainlayout_obj mainlayout;
        struct mainpage_obj mainpage;
        struct menu_obj menu;
        struct datalog_obj  datalog;
        struct modelcfg_obj modelcfg;
        struct modelload_obj modelload;
        struct modelpage_obj modelpage;
        struct reorder_obj reorder;
#if SUPPORT_SCANNER
        struct scanner_obj scanner;
#endif
        struct telemcfg_obj telemcfg;
        struct telemtest_obj telemtest1;
        struct timer_obj timer;
        struct settimer_obj settimer;
        struct trim_obj trim;
        struct trimedit_obj trimedit;
        struct tx_obj tx;
        struct calibrate_obj calibrate;
        struct usb_obj usb;
        struct rtc_obj rtc;
        struct debuglog_obj debuglog;
#ifdef HAS_MUSIC_CONFIG
        struct voiceconfig_obj voiceconfig;
#endif

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
};

extern struct _gui_objs gui_objs;
#endif
