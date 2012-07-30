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

#include "target.h"
#include "pages.h"
#include "icons.h"
#include "gui/gui.h"

struct page {
    void (*init)(int i);
    void (*event)();
    void (*exit)();
};

struct pagemem pagemem;

static const struct page pages[] = {
    {PAGE_MainInit, PAGE_MainEvent, PAGE_MainExit},
    {NULL, NULL, NULL},
    {PAGE_MixerInit, PAGE_MixerEvent, NULL},
    {PAGE_TrimInit, PAGE_TrimEvent, NULL},
    {PAGE_ModelInit, PAGE_ModelEvent, NULL},
    {PAGE_TimerInit, PAGE_TimerEvent, NULL},
    {NULL, NULL, NULL},
    {PAGE_MainCfgInit, PAGE_MainCfgEvent, NULL},
    {PAGE_ChantestInit, PAGE_ChantestEvent, NULL},
    {PAGE_ScannerInit, PAGE_ScannerEvent, NULL},
    //{PAGE_TestInit, PAGE_TestEvent, NULL},
    {PAGE_USBInit, PAGE_USBEvent, PAGE_USBExit},
    {NULL, NULL, NULL},
};

static u8 page;
static u8 modal;
void PAGE_Init()
{
    page = 0;
    modal = 0;
    GUI_RemoveAllObjects();
    pages[page].init(0);
}

void PAGE_SetSection(u8 section)
{
    u8 p;
    u8 newpage = page;
    u8 sec = 0;
    for(p = 0; p < sizeof(pages) / sizeof(struct page); p++) {
        if(sec == section) {
            newpage = p;
            break;
        }
        if (pages[p].init == NULL)
            sec++;
    }
    if (newpage != page) {
        if (pages[page].exit)
            pages[page].exit();
        page = newpage;
        GUI_RemoveAllObjects();
        pages[page].init(0);
    }
}

void PAGE_Change(int dir)
{
    if ( modal || GUI_IsModal())
        return;
    u8 nextpage = page;
    if(dir > 0) {
        if (pages[nextpage+1].init != NULL) {
            nextpage++;
        } else {
            while(nextpage && pages[nextpage-1].init != NULL)
              nextpage--;
        } 
    } else if (dir < 0) {
        if (nextpage && pages[nextpage-1].init != NULL) {
            nextpage--;
        } else {
            while(pages[nextpage+1].init != NULL)
                nextpage++;
        }
    }
    if (page == nextpage)
        return;
    if (pages[page].exit)
        pages[page].exit();
    page = nextpage;
    GUI_RemoveAllObjects();
    pages[page].init(0);
}

void PAGE_Event()
{
    pages[page].event();
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
void changepage_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    if((long)data == 0) {
        PAGE_SetSection(SECTION_MAIN);
    } else if ((long)data == 1) {
        PAGE_Change(1);
    } else if ((long)data == -1) {
        PAGE_Change(-1);
    }
}
void PAGE_ShowHeader(const char *title)
{
    guiObject_t *obj;
    GUI_CreateIcon(0, 1, &icons[ICON_EXIT], changepage_cb, (void *)0);
    GUI_CreateLabel(40, 10, NULL, TITLE_FONT, (void *)title);
    obj = GUI_CreateIcon(254, 1, &icons[ICON_PREVPAGE], changepage_cb, (void *)-1);
    GUI_SetSelectable(obj, 0);
    obj = GUI_CreateIcon(288, 1, &icons[ICON_NEXTPAGE], changepage_cb, (void *)1);
    GUI_SetSelectable(obj, 0);
}

static const char *okcancelstr_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    return data ? "OK" : "Cancel";
}

guiObject_t *PAGE_CreateCancelButton(u16 x, u16 y, void (*CallBack)(guiObject_t *obj, void *data))
{
    return GUI_CreateButton(x, y, BUTTON_96, okcancelstr_cb, 0x0000, CallBack, (void *)0);
}
guiObject_t *PAGE_CreateOkButton(u16 x, u16 y, void (*CallBack)(guiObject_t *obj, void *data))
{
    return GUI_CreateButton(x, y, BUTTON_48, okcancelstr_cb, 0x0000, CallBack, (void *)1);
}
