#ifndef _PAGES_H_
#define _PAGES_H_

#include "../common/mixer_page.h"
#include "../common/main_page.h"
#include "../common/trim_page.h"
#include "../common/timer_page.h"
#include "../common/model_page.h"
#include "../common/chantest_page.h"
//#include "scanner_page.h"
#include "../common/usb_page.h"
#include "../common/tx_configure.h"
#include "../common/telemtest_page.h"
#include "../common/telemconfig_page.h"
#include "config/display.h"

#include "menus.h"

#define PAGEDEF(id, init, event, exit, name) id,
enum PageID {
#include "pagelist.h"
};
#undef PAGEDEF

struct pagemem {
    union {
        struct main_page main_page;
        struct mixer_page mixer_page;
        struct trim_page trim_page;
        struct model_page model_page;
        struct timer_page timer_page;
        struct chantest_page chantest_page;
        //struct scanner_page scanner_page;
        struct usb_page usb_page;
        struct tx_configure_page tx_configure_page;
        struct telemtest_page telemtest_page;
        struct telemconfig_page telemconfig_page;

        struct menu_page menu_page;
    } u;
    u8 modal_page;
};
#define PAGE_NAME_MAX 10
#define ITEM_HEIGHT 12
#define ITEM_SPACE 13
#define PREVIOUS_ITEM -1
#define PAGE_ITEM_MAX 4

extern struct pagemem pagemem;
extern struct LabelDesc labelDesc;

void PAGE_ShowHeader(const char *title);
void PAGE_ShowHeaderWithHeight(const char *title, u8 font, u8 width, u8 height);
void PAGE_ShowHeader_ExitOnly(const char *str, void (*CallBack)(guiObject_t *obj, const void *data));
u8 PAGE_SetModal(u8 _modal);
u8 PAGE_GetModal();
void PAGE_RemoveAllObjects();
guiObject_t *PAGE_CreateCancelButton(u16 x, u16 y, void (*CallBack)(guiObject_t *obj, const void *data));
guiObject_t *PAGE_CreateOkButton(u16 x, u16 y, void (*CallBack)(guiObject_t *obj, const void *data));
void PAGE_ChangeByID(enum PageID id, s8 page);
void PAGE_SetActionCB(u8 (*callback)(u32 button, u8 flags, void *data));
void PAGE_NavigateItems(s8 direction, u8 view_id, u8 total_items, s8 *selectedIdx, s16 *view_origin_relativeY,
        guiObject_t *scroll_bar);

// Main
void PAGE_MainInit(int page);
void PAGE_MainEvent();
void PAGE_MainExit();

// Menu
void PAGE_MenuInit(int page);
// Main Menu
void PAGE_MainMenuInit(int page);
void PAGE_MainMenuExit();

// Sub Menu
void PAGE_SubMenuInit(int page);
void PAGE_SubMenuExit();

// Mixer
void PAGE_MixerInit(int page);
void PAGE_MixerEvent();

// Trim
void PAGE_TrimInit(int page);
void PAGE_TrimEvent();

// Timer
void PAGE_TimerInit(int page);
void PAGE_TimerEvent();

// Model
void PAGE_ModelInit(int page);
void PAGE_ModelEvent();
void MODELPage_ShowLoadSave(int loadsave, void(*return_page)(int page));
void MODELPAGE_Config();
void MODELPage_Template();

// Test
void PAGE_TestInit(int page);
void PAGE_TestEvent();

// Chantest
void PAGE_ChantestInit(int page);
void PAGE_InputtestInit(int page);
void PAGE_ButtontestInit(int page);
void PAGE_ChantestEvent();
void PAGE_ChantestExit();
void PAGE_ChantestModal(void(*return_page)(int page), int page);

// Scanner
void PAGE_ScannerInit(int page);
void PAGE_ScannerEvent();
void PAGE_ScannerExit();

// USB
void PAGE_USBInit(int page);
void PAGE_USBEvent();
void PAGE_USBExit();

// Calibrate
void PAGE_TxConfigureInit(int page);
void PAGE_TxConfigureEvent();
void LANGPage_Select(void(*return_page)(int page));

void PAGE_MainCfgEvent();
void PAGE_MainCfgInit(int page);

/* Telemetry Test */
void PAGE_TelemtestInit(int page);
void PAGE_TelemtestGPSInit(int page);
void PAGE_TelemtestEvent();
void PAGE_TelemtestModal(void(*return_page)(int page), int page);

/* Telemetry Config */
void PAGE_TelemconfigInit(int page);
void PAGE_TelemconfigEvent();

void PAGE_AboutInit(int page);

int PAGE_QuickPage(u32 buttons, u8 flags, void *data);
#endif
