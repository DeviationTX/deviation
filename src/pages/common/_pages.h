#include "common.h"
#include "music.h"
#include "gui/gui.h"
#include "config/model.h"
#include <stdlib.h>

#include "mixer_page.h"
#include "main_page.h"
#include "trim_page.h"
#include "timer_page.h"
#include "model_page.h"
#include "chantest_page.h"
#include "usb_page.h"
#include "tx_configure.h"
#include "telemtest_page.h"
#include "telemconfig_page.h"
#include "config/display.h"

#define PAGE_NAME_MAX 10
extern struct pagemem pagemem;

void PAGE_ShowHeader(const char *title);
void PAGE_ShowHeader_ExitOnly(const char *str, void (*CallBack)(guiObject_t *obj, const void *data));
u8 PAGE_SetModal(u8 _modal);
u8 PAGE_GetModal();
void PAGE_RemoveAllObjects();
guiObject_t *PAGE_CreateCancelButton(u16 x, u16 y, void (*CallBack)(guiObject_t *obj, const void *data));
guiObject_t *PAGE_CreateOkButton(u16 x, u16 y, void (*CallBack)(guiObject_t *obj, const void *data));
void PAGE_SetActionCB(u8 (*callback)(u32 button, u8 flags, void *data));

/* Main */
void PAGE_MainInit(int page);
void PAGE_MainEvent();
void PAGE_MainExit();

/* Mixer */
void PAGE_MixerInit(int page);
void PAGE_MixerEvent();

/* Trim */
void PAGE_TrimInit(int page);
void PAGE_TrimEvent();

/* Timer */
void PAGE_TimerInit(int page);
void PAGE_TimerEvent();

/* Model */
void PAGE_ModelInit(int page);
void PAGE_ModelEvent();
void MODELPage_ShowLoadSave(int loadsave, void(*return_page)(int page));
void MODELPAGE_Config();
void MODELPROTO_Config();
void MODELPage_Template();

/* Test */
void PAGE_TestInit(int page);
void PAGE_TestEvent();

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

/* USB */
void PAGE_TxConfigureInit(int page);
void PAGE_TxConfigureEvent();
void LANGPage_Select(void(*return_page)(int page));

void PAGE_MainCfgEvent();
void PAGE_MainCfgInit(int page);

/* Telemetry Test */
void PAGE_TelemtestInit(int page);
void PAGE_TelemtestEvent();
void PAGE_TelemtestModal(void(*return_page)(int page), int page);

/* Telemetry Config */
void PAGE_TelemconfigInit(int page);
void PAGE_TelemconfigEvent();

int PAGE_QuickPage(u32 buttons, u8 flags, void *data);
u8 PAGE_TelemStateCheck(char *str, int strlen);


/* Simple Mixer pages */
void PAGE_ReverseInit(int page);
void PAGE_TravelAdjInit(int page);
void PAGE_PitCurvesInit(int page);
void PAGE_ThroCurvesInit(int page);
void PAGE_SubtrimInit(int page);
void PAGE_ThroHoldInit(int page);
void PAGE_DrExpInit(int page);
void PAGE_SwashInit(int page);
void PAGE_GyroSenseInit(int page);
void PAGE_CurvesEvent();
void PAGE_SwitchAssignInit(int page);
void PAGE_FailSafeInit(int page);

