#ifndef _USB_PAGE_H_
#define _USB_PAGE_H_
#include "buttons.h"

struct usb_page {
    struct buttonAction action;
    char tmpstr[200];
#if HAS_RTC
    u32  timeval;
    char datetime[12];
#endif
};
#endif
