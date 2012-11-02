#ifndef _PAGES_H_
#define _PAGES_H_

#include "../common/mixer_page.h"
#include "../_trim_page.h"
#include "../_timer_page.h"
#include "../_model_page.h"
#include "../common/chantest_page.h"
#include "../common/tx_configure.h"
#include "scanner_page.h"
#include "usb_page.h"
#include "telemtest_page.h"
#include "config/display.h"

#include "main_page.h"
#include "main_menu.h"
#include "sub_menu.h"
#include "single_itemconfig.h"
#include "multi_items_config.h"

struct pagemem {
    union {
        struct main_page main_page;
        struct main_menu_page main_menu_page;
        struct sub_menu_page sub_menu_page;
        struct chantest_page chantest_page;
        struct tx_configure_page tx_configure_page;
        struct single_itemCofig_page single_itemCofig_page;
        struct multi_items_config_page multi_items_config_page;


        struct telemtest_page telemtest_page;
        struct mixer_page mixer_page;
        struct trim_page trim_page;
        struct model_page model_page;
        struct timer_page timer_page;
        struct scanner_page scanner_page;
        struct usb_page usb_page;
    } u;
};
#define PAGE_NAME_MAX 10
#define ITEM_HEIGHT 12

extern struct pagemem pagemem;
extern u8 sub_menu_item;  // global variable to let other page get back to the right sub menu

void PAGE_ShowHeader(const char *title);
void PAGE_ShowHeaderWithHeight(const char *title, u8 font, u8 width, u8 height);
void PAGE_ShowHeader_ExitOnly(const char *str, void (*CallBack)(guiObject_t *obj, const void *data));
u8 PAGE_SetModal(u8 _modal);
u8 PAGE_GetModal();
void PAGE_RemoveAllObjects();
guiObject_t *PAGE_CreateCancelButton(u16 x, u16 y, void (*CallBack)(guiObject_t *obj, const void *data));
guiObject_t *PAGE_CreateOkButton(u16 x, u16 y, void (*CallBack)(guiObject_t *obj, const void *data));
void PAGE_ChangeByName(const char *pageName, u8 page);
void PAGE_SetActionCB(u8 (*callback)(u32 button, u8 flags, void *data));
void PAGE_NavigateItems(s8 direction, u8 view_id, u8 total_items, s8 *selectedIdx, s16 *view_origin_relativeY,
        guiObject_t *scroll_bar);

// Main
void PAGE_MainInit(int page);
void PAGE_MainEvent();
void PAGE_MainExit();

// Main Menu
void PAGE_MainMenuInit(int page);
void PAGE_MainMenuExit();

// Sub Menu
void PAGE_SubMenuInit(int page);
void PAGE_SubMenuExit();

// Language select
void PAGE_SingleItemConfigInit();
void PAGE_SingleItemConfigExit();

void PAGE_MultiItemsConfigInit();
void PAGE_MultiItemsConfigExit();

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
void PAGE_CalibrateInit(int page);
void PAGE_CalibrateEvent();
void LANGPage_Select(void(*return_page)(int page));

void PAGE_MainCfgEvent();
void PAGE_MainCfgInit(int page);

/* Telemetry Test */
void PAGE_TelemtestInit(int page);
void PAGE_TelemtestEvent();
#endif
