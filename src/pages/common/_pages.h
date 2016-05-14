#ifndef __PAGES_H__
#define __PAGES_H__
#include "common.h"
#include "music.h"
#include "gui/gui.h"
#include "config/model.h"
#include <stdlib.h>

#include "mixer_page.h"
#include "main_page.h"
#include "main_layout.h"
#include "trim_page.h"
#include "timer_page.h"
#include "model_page.h"
#include "chantest_page.h"
#include "range_page.h"
#include "usb_page.h"
#include "tx_configure.h"
#include "telemtest_page.h"
#include "telemconfig_page.h"
#include "toggle_select.h"
#include "_menus.h"
#include "config/display.h"
#include "rtc_config.h"

#define PAGE_NAME_MAX 10

#define PAGEDEF(id, init, event, exit, menu, name) id,
enum PageID {
#include "pagelist.h"
PAGEID_LAST
};
#undef PAGEDEF

extern struct pagemem pagemem;

void PAGE_RemoveHeader();
void PAGE_ShowHeader(const char *title);
void PAGE_ShowHeader_SetLabel(const char *(*label_cb)(guiObject_t *obj, const void *data), void *data);

u8 PAGE_SetModal(u8 _modal);
u8 PAGE_GetModal();
void PAGE_RemoveAllObjects();
guiObject_t *PAGE_CreateCancelButton(u16 x, u16 y, void (*CallBack)(guiObject_t *obj, const void *data));
guiObject_t *PAGE_CreateOkButton(u16 x, u16 y, void (*CallBack)(guiObject_t *obj, const void *data));
void PAGE_SetActionCB(unsigned (*callback)(u32 button, unsigned flags, void *data));
void PAGE_SetScrollable(guiScrollable_t *scroll, u16 *selected);
void PAGE_SaveCurrentPos();

/* Main */
void PAGE_MainInit(int page);
void PAGE_MainEvent();
void PAGE_MainExit();
struct ImageMap TGLICO_GetImage(int idx);
void TGLICO_Select(guiObject_t *obj, const void *data);

/* Mixer */
void PAGE_MixerInit(int page);
void PAGE_MixerExit();
void PAGE_EditLimitsInit(int page);
void PAGE_MixReorderInit(int page);
void PAGE_MixTemplateInit(int page);
void PAGE_MixTemplateEvent();
void PAGE_EditCurvesInit(int page);

/* Trim */
void PAGE_TrimInit(int page);
void PAGE_TrimEvent();
void PAGE_TrimExit();

/* Timer */
void PAGE_TimerInit(int page);
void PAGE_TimerEvent();
void PAGE_TimerExit();
void TIMERPAGE_Show(guiObject_t *obj, const void *data);
/* Set permanent timer */
void PAGE_SetTimerInit(int index);
void PAGE_SetTimerExit();

/* Model */
void PAGE_ModelInit(int page);
void PAGE_ModelEvent();
void PAGE_ModelExit();
void MODELPage_ShowLoadSave(int loadsave, void(*return_page)(int page));
void MODELPAGE_Config();
void MODELPROTO_Config();
void MODELTRAIN_Config();
void MODELVIDEO_Config();
void MODELPage_Template();

/* RTC */
void PAGE_RTCInit(int page);
void PAGE_RTCEvent();
void PAGE_RTCExit();

/* Test */
void PAGE_TestInit(int page);
void PAGE_TestEvent();

/* Range */
void PAGE_RangeInit(int page);
void PAGE_RangeEvent();
void PAGE_RangeExit();

/* Chantest */
void PAGE_ChantestInit(int page);
void PAGE_InputtestInit(int page);
void PAGE_ButtontestInit(int page);
void PAGE_ChantestEvent();
void PAGE_ChantestExit();
void PAGE_ChantestModal(void(*return_page)(int page), int page);

/* Scanner */
void PAGE_ScannerInit(int page);
void PAGE_ScannerEvent();
void PAGE_ScannerExit();

/* USB */
void PAGE_USBInit(int page);
void PAGE_USBEvent();
void PAGE_USBExit();

/* TX configure */
void PAGE_TxConfigureInit(int page);
void PAGE_TxConfigureEvent();
void LANGPage_Select(void(*return_page)(int page));
void PAGE_TxConfigureExit();

void PAGE_MainCfgEvent();
void PAGE_MainCfgInit(int page);
void PAGE_MainCfgExit();
void PAGE_MainLayoutInit(int page);
void PAGE_MainLayoutEvent();
void PAGE_MainLayoutExit();

/* Telemetry Test */
void PAGE_TelemtestInit(int page);
void PAGE_TelemtestEvent();
void PAGE_TelemtestModal(void(*return_page)(int page), int page);

/* Telemetry Config */
void PAGE_TelemconfigInit(int page);
void PAGE_TelemconfigEvent();
void PAGE_TelemconfigExit();

/* Datalog */
void PAGE_DatalogInit();
void PAGE_DatalogEvent();
void PAGE_DatalogExit();

/* Debuglog */
void PAGE_DebuglogInit();
void PAGE_DebuglogEvent();
void PAGE_DebuglogExit();

void PAGE_ChangeByID(enum PageID id, s8 menuPage);
void PAGE_PushByID(enum PageID id, int page);
void PAGE_Pop();

int PAGE_QuickPage(u32 buttons, u8 flags, void *data);
u8 PAGE_TelemStateCheck(char *str, int strlen);
int PAGE_IsValidQuickPage(int page);


/* Simple Mixer pages */
void PAGE_ModelMenuInit(int page);
void PAGE_ModelMenuExit();
void PAGE_ReverseInit(int page);
void PAGE_TravelAdjInit(int page);
void PAGE_TravelAdjExit();
void PAGE_PitCurvesInit(int page);
void PAGE_ThroCurvesInit(int page);
void PAGE_SubtrimInit(int page);
void PAGE_ThroHoldInit(int page);
void PAGE_DrExpInit(int page);
void PAGE_SwashInit(int page);
void PAGE_GyroSenseInit(int page);
void PAGE_CurvesEvent();
void PAGE_DrExpCurvesEvent();
void PAGE_SwitchAssignInit(int page);
void PAGE_FailSafeInit(int page);

#endif //__PAGES_H__
