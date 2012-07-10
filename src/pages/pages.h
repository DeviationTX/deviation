#ifndef _PAGES_H_
#define _PAGES_H_

#include "mixer_page.h"
#include "main_page.h"
#include "trim_page.h"
#include "model_page.h"

struct pagemem {
    union {
        struct main_page main_page;
        struct mixer_page mixer_page;
        struct trim_page trim_page;
        struct model_page model_page;
    } u;
};

extern struct pagemem pagemem;

/* Main */
void PAGE_MainInit(int page);
void PAGE_MainEvent();
int PAGE_MainCanChange();

/* Mixer */
void PAGE_MixerInit(int page);
void PAGE_MixerEvent();
int PAGE_MixerCanChange();

/* Trim */
void PAGE_TrimInit(int page);
void PAGE_TrimEvent();
int PAGE_TrimCanChange();

/* Model */
void PAGE_ModelInit(int page);
void PAGE_ModelEvent();
int PAGE_ModelCanChange();

/* Test */
void PAGE_TestInit(int page);
void PAGE_TestEvent();
int PAGE_TestCanChange();

/* Chantest */
void PAGE_ChantestInit(int page);
void PAGE_ChantestEvent();
int PAGE_ChantestCanChange();

/* Scanner */
void PAGE_ScannerInit(int page);
void PAGE_ScannerEvent();
int PAGE_ScannerCanChange();

/* USB */
void PAGE_USBInit(int page);
void PAGE_USBEvent();
int PAGE_USBCanChange();
#endif
