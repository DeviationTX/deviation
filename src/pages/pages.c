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
#include "gui/gui.h"

struct page {
    void (*init)(int i);
    void (*event)();
};

struct pagemem pagemem;

static const struct page pages[] = {
    {PAGE_MainInit, PAGE_MainEvent},
    {PAGE_MixerInit, PAGE_MixerEvent},
    {PAGE_TrimInit, PAGE_TrimEvent},
    {PAGE_TimerInit, PAGE_TimerEvent},
    {PAGE_ModelInit, PAGE_ModelEvent},
    {PAGE_ChantestInit, PAGE_ChantestEvent},
    {PAGE_ScannerInit, PAGE_ScannerEvent},
    {PAGE_TestInit, PAGE_TestEvent},
    {PAGE_USBInit, PAGE_USBEvent},
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

void PAGE_Change(int dir)
{
    if ( modal || GUI_IsModal())
        return;
    if (dir == 0)
        return;
    GUI_RemoveAllObjects();
    if(dir > 0) {
        page++;
        if (page >= sizeof(pages) / sizeof(struct page))
            page = 0;
    } else if (dir < 0) {
        if (page == 0)
            page = sizeof(pages) / sizeof(struct page) - 1;
        else
            page--;
    }
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
