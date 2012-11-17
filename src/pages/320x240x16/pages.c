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

#include "common.h"
#include "pages.h"
#include "icons.h"
#include "gui/gui.h"

static buttonAction_t button_action;
static u8 (*ActionCB)(u32 button, u8 flags, void *data);

static void (*enter_cmd)(guiObject_t *obj, const void *data);
static const void *enter_data;
static void (*exit_cmd)(guiObject_t *obj, const void *data);
static const void *exit_data;
static u8 page_change_cb(u32 buttons, u8 flags, void *data);
static void PAGE_SwitchByName(const char *name);
void PAGE_Exit();

#define PAGE_NAME_MAX 10
struct page {
    void (*init)(int i);
    void (*event)();
    void (*exit)();
    const char *pageName;
};

struct pagemem pagemem;

static const struct page pages[] = {
    {PAGE_MainInit, PAGE_MainEvent, PAGE_MainExit, "MainPage"},
    {PAGE_MixerInit, PAGE_MixerEvent, NULL, "Mixer"},
    {PAGE_ModelInit, PAGE_ModelEvent, NULL, "ModelCon"},
    {PAGE_TimerInit, PAGE_TimerEvent, NULL, "Timer"},
    {PAGE_TelemconfigInit, PAGE_TelemconfigEvent, NULL, "TeleConf"},
    {PAGE_TrimInit, PAGE_TrimEvent, NULL, "Trim"},
    {PAGE_MainCfgInit, PAGE_MainCfgEvent, NULL, "MainConf"},
    {PAGE_TxConfigureInit, PAGE_TxConfigureEvent, NULL, "TxConfig"},
    {PAGE_TelemtestInit, PAGE_TelemtestEvent, NULL, "TeleMoni"},
    {PAGE_ChantestInit, PAGE_ChantestEvent, PAGE_ChantestExit, "Monitor"},
    {PAGE_InputtestInit, PAGE_ChantestEvent, PAGE_ChantestExit, "InputMon"},
    {PAGE_ButtontestInit, PAGE_ChantestEvent, PAGE_ChantestExit, "ButtonMon"},
    {PAGE_ScannerInit, PAGE_ScannerEvent, PAGE_ScannerExit, "Scanner"},
    //{PAGE_TestInit, PAGE_TestEvent, NULL},
    {PAGE_USBInit, PAGE_USBEvent, PAGE_USBExit, "USB"},
    {NULL, NULL, NULL, NULL},
};

static const struct page *curpage;

struct page_group {
    u8 group;
    const char *name;
};

struct page_group groups[] = {
    {0, "MainPage"},
    {1, "Mixer"},
    {1, "ModelCon"},
    {1, "Timer"},
    {1, "TeleConf"},
    {1, "Trim"},
    {1, "MainConf"},
    {2, "TxConfig"},
    {2, "TeleMoni"},
    {2, "Monitor"},
    {2, "InputMon"},
    {2, "ButtonMon"},
    {2, "Scanner"},
    {2, "USB"},
    {255, NULL}
};
static u8 page;
static u8 modal;
void PAGE_Init()
{
    page = 0;
    modal = 0;
    GUI_RemoveAllObjects();
    enter_cmd = NULL;
    exit_cmd = NULL;
    BUTTON_RegisterCallback(&button_action,
        CHAN_ButtonMask(BUT_ENTER) | CHAN_ButtonMask(BUT_EXIT)
        | CHAN_ButtonMask(BUT_RIGHT) | CHAN_ButtonMask(BUT_LEFT),
        BUTTON_PRESS | BUTTON_LONGPRESS, page_change_cb, NULL);
    PAGE_SwitchByName("MainPage");
}

void PAGE_SetSection(u8 section)
{
    u8 p;
    u8 newpage = page;
    for(p = 0; groups[p].name != 0; p++) {
        if(groups[p].group == section) {
            newpage = p;
            break;
        }
    }
    if (newpage != page) {
        PAGE_Exit();
        page = newpage;
        PAGE_RemoveAllObjects();
        PAGE_SwitchByName(groups[page].name);
    }
}

void PAGE_Change(int dir)
{
    if ( modal || GUI_IsModal())
        return;
    u8 nextpage = page;
    if(dir > 0) {
        if (groups[nextpage+1].name != NULL && groups[nextpage+1].group == groups[page].group) {
            nextpage++;
        } else {
            while(nextpage && groups[nextpage-1].group == groups[page].group)
              nextpage--;
        } 
    } else if (dir < 0) {
        if (nextpage && groups[nextpage-1].group == groups[page].group) {
            nextpage--;
        } else {
            while(groups[nextpage+1].name != NULL && groups[nextpage+1].group == groups[page].group)
                nextpage++;
        }
    }
    if (page == nextpage)
        return;
    PAGE_Exit();
    page = nextpage;
    PAGE_RemoveAllObjects();
    PAGE_SwitchByName(groups[page].name);
}

void PAGE_SwitchByName(const char *name)
{
    int i = 0;
    while(pages[i].init) {
        if(strcmp(name, pages[i].pageName) == 0) {
            curpage = &pages[i];
            curpage->init(0);
        }
        i++;
    }
}

void PAGE_Event()
{
    if(curpage && curpage->event)
        curpage->event();
}

void PAGE_Exit()
{
    if(curpage && curpage->exit)
        curpage->exit();
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
void changepage_cb(guiObject_t *obj, const void *data)
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
    GUI_CreateIcon(0, 0, &icons[ICON_EXIT], changepage_cb, (void *)0);
    GUI_CreateLabel(40, 10, NULL, TITLE_FONT, (void *)title);
    obj = GUI_CreateIcon(256, 0, &icons[ICON_PREVPAGE], changepage_cb, (void *)-1);
    GUI_SetSelectable(obj, 0);
    obj = GUI_CreateIcon(288, 0, &icons[ICON_NEXTPAGE], changepage_cb, (void *)1);
    GUI_SetSelectable(obj, 0);
    exit_cmd = changepage_cb;
    exit_data = NULL;
}

void PAGE_ShowHeader_ExitOnly(const char *title, void (*CallBack)(guiObject_t *obj, const void *data))
{
    enter_cmd = CallBack;
    enter_data = (void *)1;
    exit_cmd = CallBack;
    exit_data = (void *)0;
    GUI_CreateIcon(0, 0, &icons[ICON_EXIT], CallBack, (void *)0);
    GUI_CreateLabel(40, 10, NULL, TITLE_FONT, (void *)title);
}

static const char *okcancelstr_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    return data ? _tr("Ok") : _tr("Cancel");
}

void PAGE_SetActionCB(u8 (*callback)(u32 button, u8 flags, void *data))
{
    ActionCB = callback;
}

u8 page_change_cb(u32 buttons, u8 flags, void *data)
{
    (void)data;
    (void)flags;
    if (ActionCB != NULL)
        return ActionCB(buttons, flags, data);

    if (flags & BUTTON_LONGPRESS) {
        if (flags & BUTTON_REPEAT)
            return 0;
        if(CHAN_ButtonIsPressed(buttons, BUT_ENTER) && enter_cmd) {
            void (*cmd)(guiObject_t *obj, const void *data) = enter_cmd;
            PAGE_RemoveAllObjects();
            cmd(NULL, enter_data);
            return 1;
        }
        if(CHAN_ButtonIsPressed(buttons, BUT_EXIT) && exit_cmd) {
            void (*cmd)(guiObject_t *obj, const void *data) = exit_cmd;
            PAGE_RemoveAllObjects();
            cmd(NULL, exit_data);
            return 1;
        }
        return 0;
    }
    if(CHAN_ButtonIsPressed(buttons, BUT_RIGHT)) {
        PAGE_Change(1);
        return 1;
    } else if(CHAN_ButtonIsPressed(buttons, BUT_LEFT)) {
        PAGE_Change(-1);
        return 1;
    }
    return 0;
}

void PAGE_RemoveAllObjects()
{
    enter_cmd = NULL;
    exit_cmd = NULL;
    GUI_RemoveAllObjects();
}

guiObject_t *PAGE_CreateCancelButton(u16 x, u16 y, void (*CallBack)(guiObject_t *obj, const void *data))
{
    exit_cmd = CallBack;
    exit_data = (void *)0;
    return GUI_CreateButton(x, y, BUTTON_96, okcancelstr_cb, 0x0000, CallBack, (void *)0);
}
guiObject_t *PAGE_CreateOkButton(u16 x, u16 y, void (*CallBack)(guiObject_t *obj, const void *data))
{
    enter_cmd = CallBack;
    enter_data = (void *)1;
    return GUI_CreateButton(x, y, BUTTON_48, okcancelstr_cb, 0x0000, CallBack, (void *)1);
}

