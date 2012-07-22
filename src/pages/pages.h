#ifndef _PAGES_H_
#define _PAGES_H_

#include "mixer_page.h"
#include "main_page.h"
#include "trim_page.h"
#include "timer_page.h"
#include "model_page.h"
#include "chantest_page.h"

struct pagemem {
    union {
        struct main_page main_page;
        struct mixer_page mixer_page;
        struct trim_page trim_page;
        struct model_page model_page;
        struct timer_page timer_page;
        struct chantest_page chantest_page;
    } u;
};

extern struct pagemem pagemem;

/* Main */
void PAGE_MainInit(int page);
void PAGE_MainEvent();
u8 PAGE_SetModal(u8 _modal);

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

/* Test */
void PAGE_TestInit(int page);
void PAGE_TestEvent();

/* Chantest */
void PAGE_ChantestInit(int page);
void PAGE_ChantestEvent();

/* Scanner */
void PAGE_ScannerInit(int page);
void PAGE_ScannerEvent();

/* USB */
void PAGE_USBInit(int page);
void PAGE_USBEvent();
#endif
