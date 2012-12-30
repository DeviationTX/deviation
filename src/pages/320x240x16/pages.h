#ifndef _PAGES_H_
#define _PAGES_H_

#include "../common/_pages.h"
#include "scanner_page.h"
#include "icons.h"

#define PAGEDEF(id, init, event, exit, name) id,
enum PageID {
#include "pagelist.h"
};
#undef PAGEDEF

#define SECTION_MAIN    0
#define SECTION_MODEL   1
#define SECTION_OPTIONS 2
struct pagemem {
    union {
        struct main_page main_page;
        struct mixer_page mixer_page;
        struct trim_page trim_page;
        struct model_page model_page;
        struct timer_page timer_page;
        struct chantest_page chantest_page;
        struct scanner_page scanner_page;
        struct usb_page usb_page;
        struct tx_configure_page tx_configure_page;
        struct telemtest_page telemtest_page;
        struct telemconfig_page telemconfig_page;
    } u;
    u8 modal_page;
};

#define TOGGLE_FILE "media/toggle.bmp"
#define MODELMENU_FILE "media/modelmnu.bmp"

void PAGE_SetSection(u8 section);
void PAGE_ChangeByID(enum PageID id);
void MODELMENU_Show(guiObject_t *obj, const void *data);
#endif
