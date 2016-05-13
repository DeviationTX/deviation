/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */

static buttonAction_t button_action;
static unsigned (*ActionCB)(u32 button, unsigned flags, void *data);

static u8 _page_stack[5];
static u8 *page_stack = _page_stack;

void PAGE_ChangeQuick(int dir);

struct pagemem pagemem;
static u8 modal;
static u8 cur_page;

struct page {
    void (*init)(int i);
    void (*event)();
    void (*exit)();
    const char *pageName;
};

#define PAGEDEF(id, init, event, exit, menu, name) {init, event, exit, name},
static const struct page pages[] = {
#include "pagelist.h"
};
#undef PAGEDEF

static unsigned _action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_Pop();
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

void PAGE_Event()
{
    if(pages[cur_page].event)
        pages[cur_page].event();
}

void PAGE_Exit()
{
    if(pages[cur_page].exit)
        pages[cur_page].exit();
}

// Pages that need special handling before saving on power off or battery low
void PAGE_Test(void)
{
    if (cur_page == PAGEID_RANGE) {
        PAGE_RangeExit();
        PAGE_RangeInit(0);
    }
}


u8 PAGE_SetModal(u8 _modal)
{
    u8 old = modal;
    modal = _modal;
    return old;
}

u8 PAGE_GetModal()
{
    return modal;
}


void PAGE_SetActionCB(unsigned (*callback)(u32 button, unsigned flags, void *data))
{
    ActionCB = callback;
}

u8 PAGE_TelemStateCheck(char *str, int strlen)
{
    (void)strlen;
    s8 state = PROTOCOL_GetTelemetryState();
    if (state == -1) {
        sprintf(str, "%s%s%s",
            _tr("Telemetry"),
            LCD_DEPTH == 1?"\n":" ", // no translate for this string
            _tr("is not supported"));
        return 0;
    }
    else if (state == 0) {
        sprintf(str, "%s%s%s",
            _tr("Telemetry"),
            LCD_DEPTH == 1?"\n":" ",  // no translate for this string
            _tr("is turned off"));
        return 0;
    }
    return 1;
}

int PAGE_IsValidQuickPage(int page) {
#if HAS_STANDARD_GUI
    int menu;
    #define PAGEDEF(_id, _init, _event, _exit, _menu, _name) \
        case _id: menu = _menu; break;
    switch(page) {
        #include <pagelist.h>
    }
    #undef PAGEDEF
    if (! (menu & Model.mixer_mode)) {
        return 0;
    }
#endif //HAS_STANDARD_GUI
    switch(page) {
        case PAGEID_SPLASH:
            return 0;
    }
    return 1;
}

const char *PAGE_GetName(int i)
{
    return _tr(pages[i].pageName);
}

int PAGE_GetNumPages()
{
    return sizeof(pages) / sizeof(struct page);
}

void PAGE_PushByID(enum PageID id)
{
    page_stack++;
    *page_stack = id;
    PAGE_ChangeByID(id, 0);
}
void PAGE_Pop()
{
    //page 0 is always PAGEID_MAIN
    if (page_stack > _page_stack+1) {
        page_stack--;
        PAGE_ChangeByID(*page_stack, 0);
    } else {
        page_stack = _page_stack;
        PAGE_ChangeByID(PAGEID_MAIN, 0);
    }
}
