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
};

struct pagemem pagemem;

static const struct page pages[] = {
    {PAGE_MainInit, PAGE_MainEvent},
    {NULL, NULL},
    {PAGE_ModelInit, PAGE_ModelEvent},
    {PAGE_MixerInit, PAGE_MixerEvent},
    {PAGE_TrimInit, PAGE_TrimEvent},
    {PAGE_TimerInit, PAGE_TimerEvent},
    {NULL, NULL},
    {PAGE_ChantestInit, PAGE_ChantestEvent},
    {PAGE_ScannerInit, PAGE_ScannerEvent},
    //{PAGE_TestInit, PAGE_TestEvent},
    {PAGE_USBInit, PAGE_USBEvent},
    {NULL, NULL},
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
    u8 sec = 0;
    for(p = 0; p < sizeof(pages) / sizeof(struct page); p++) {
        if(sec == section) {
            page = p;
            break;
        }
        if (pages[p].init == NULL)
            sec++;
    }
    GUI_RemoveAllObjects();
    pages[page].init(0);
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

void changepage_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    if((long)data == 0) {
        PAGE_SetSection(SECTION_MAIN);
    } else if ((long)data == 1) {
        PAGE_Change(1);
    }
}
void PAGE_ShowHeader(const char *title)
{
    GUI_CreateIcon(0, 1, &icons[ICON_EXIT], changepage_cb, (void *)0);
    GUI_CreateLabel(40, 10, NULL, TITLE_FONT, (void *)title);
    GUI_CreateIcon(288, 1, &icons[ICON_NEXTPAGE], changepage_cb, (void *)1);
}
