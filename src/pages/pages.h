#ifndef _PAGES_H_
#define _PAGES_H_

#include "mixer_page.h"

struct pagemem {
    union {
        struct mixer_page mixer_page;
    } u;
};

extern struct pagemem pagemem;

/* Mixer */
void PAGE_MixerInit(int page);
void PAGE_MixerEvent();
int PAGE_MixerCanChange();

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

#endif
