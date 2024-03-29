#ifndef _PAGES_H_
#define _PAGES_H_

#include "../common/_pages.h"
#include "icons.h"
#include "guiobj.h"

#define SECTION_MAIN    0
#define SECTION_MODEL   1
#define SECTION_OPTIONS 2
struct pagemem {
    union {
        struct main_page main_page;
        struct layout_page layout_page;
        struct mixer_page mixer_page;
        struct mixer_failsafe_page mixer_failsafe_page;
        struct mixer_limit_page mixer_limit_page;
        struct trim_page trim_page;
        struct model_page model_page;
        struct timer_page timer_page;
        struct chantest_page chantest_page;
        struct range_page range_page;
        struct gyrosense_page gyrosense_page;
        struct switchassign_page switchassign_page;
#if SUPPORT_SCANNER
        struct scanner_page scanner_page;
#endif
#if SUPPORT_XN297DUMP
        struct xn297dump_page xn297dump_page;
#endif
#if HAS_MUSIC_CONFIG
        struct voiceconfig_page voiceconfig_page;
#endif
#if SUPPORT_CRSF_CONFIG
        struct crsfconfig_page crsfconfig_page;
        struct crsfdevice_page crsfdevice_page;
#endif
        struct usb_page usb_page;
        struct tx_configure_page tx_configure_page;
        struct telemtest_page telemtest_page;
        struct telemconfig_page telemconfig_page;
        struct toggle_select_page toggle_select_page;
        struct rtc_page rtc_page;
        struct menu_page menu_page;
    } u;
    u8 modal_page;
};

#define TOGGLE_FILE    ("media/toggle" IMG_EXT)
#define MODELMENU_FILE ("media/modelmnu" IMG_EXT)
#define SPLASH_FILE    ("media/splash" IMG_EXT)

#define TOGGLEICON_WIDTH 32
#define TOGGLEICON_HEIGHT 31

void PAGE_SplashInit(int page);
void PAGE_SplashEvent();
void PAGE_SplashExit();

// Menu
void PAGE_MenuInit(int page);
void PAGE_MenuExit();
void PAGE_TxMenuInit(int page);
void PAGE_ModelMenuInit(int page);

//Touch
void PAGE_TouchInit(int page);
void PAGE_TouchEvent();
void PAGE_CRSFdialog(void *param);
void PAGE_CRSFdialogClose();
#endif
