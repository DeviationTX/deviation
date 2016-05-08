#ifndef _PAGES_H_
#define _PAGES_H_

#include "../common/_pages.h"
#include "menus.h"
#include "guiobj.h"

#define PAGEDEF(id, init, event, exit, menu, name) id,
enum PageID {
#include "pagelist.h"
    PAGEID_LAST
};
#undef PAGEDEF

struct pagemem {
    union {
        struct main_page main_page;
        struct layout_page layout_page;
        struct mixer_page mixer_page;
        struct trim_page trim_page;
        struct model_page model_page;
        struct timer_page timer_page;
        struct chantest_page chantest_page;
        struct range_page range_page;
        //struct scanner_page scanner_page;
        struct usb_page usb_page;
        struct tx_configure_page tx_configure_page;
        struct telemtest_page telemtest_page;
        struct telemconfig_page telemconfig_page;
        struct toggle_select_page toggle_select_page;

        struct menu_page menu_page;
    } u;
    u8 modal_page;
};

#ifndef HEADER_HEIGHT
    #define HEADER_HEIGHT (Display.metrics.header_height)
#endif
#ifndef HEADER_WIDGET_HEIGHT
    #define HEADER_WIDGET_HEIGHT (Display.metrics.header_widget_height)
#endif
#ifndef LINE_HEIGHT
    #define LINE_HEIGHT (Display.metrics.line_height)
#endif
#ifndef LINE_SPACE
    #define LINE_SPACE (Display.metrics.line_space)
#endif

#define PREVIOUS_ITEM -1
#define TOGGLE_FILE    "media/switches.bmp"
#define SPLASH_FILE    "media/splash.bmp"

#define TOGGLEICON_WIDTH 8
#define TOGGLEICON_HEIGHT 11

extern struct LabelDesc labelDesc;
void PAGE_SaveMixerSetup(struct mixer_page * const mp);
void PAGE_ShowHeaderWithHeight(const char *title, u8 font, u8 width, u8 height);
void PAGE_ChangeByID(enum PageID id, s8 page);

void PAGE_NavigateItems(s8 direction, u8 view_id, u8 total_items, s8 *selectedIdx, s16 *view_origin_relativeY,
        guiScrollbar_t *scroll_bar);

// Menu
void PAGE_MenuInit(int page);
void PAGE_MenuExit();
void PAGE_TxMenuInit(int page);
void PAGE_ModelMenuInit(int page);

void PAGE_AboutInit(int page);
void PAGE_SplashInit(int page);
void PAGE_SplashEvent();
void PAGE_SplashExit();
void PAGE_VideoSetupInit(int page);

#endif
