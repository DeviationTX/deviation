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
//#include "icons.h"
#include "gui/gui.h"
#include "config/model.h"

//static buttonAction_t button_action;
struct LabelDesc labelDesc; // create a style-customizable font so that it can be shared for all pages

static void (*enter_cmd)(guiObject_t *obj, const void *data);
static const void *enter_data;
static void (*exit_cmd)(guiObject_t *obj, const void *data);
static const void *exit_data;
static u8 action_cb(u32 button, u8 flags, void *data);
void PAGE_ChangeQuick(int dir);

struct page {
    void (*init)(int i);
    void (*event)();
    void (*exit)();
    const char pageName[PAGE_NAME_MAX+1];
};

struct pagemem pagemem;

static const struct page pages[] = {
    {PAGE_MainInit, PAGE_MainEvent, PAGE_MainExit, "MainPage"},  // Note: the page name length should not exceed 10 chars
    {PAGE_MenuInit, NULL, NULL, "Menu"},
    {PAGE_ChantestInit, PAGE_ChantestEvent, PAGE_ChantestExit, "Monitor"},
    {PAGE_MixerInit, PAGE_MixerEvent, NULL, "Mixer"},
    {PAGE_TxConfigureInit, PAGE_TxConfigureEvent, NULL, "TxConfig"},
    {PAGE_ModelInit, PAGE_ModelEvent, NULL, "ModelCon"},
    {PAGE_TrimInit, PAGE_TrimEvent, NULL, "Trim"},
    {PAGE_TimerInit, PAGE_TimerEvent, NULL, "Timer"},
    {PAGE_USBInit, PAGE_USBEvent, PAGE_USBExit, "USB"},
    {PAGE_TelemconfigInit, PAGE_TelemconfigEvent, NULL, "TeleConf"},
    {PAGE_TelemtestInit, PAGE_TelemtestEvent, NULL, "TeleMoni"},
    {PAGE_TelemtestGPSInit, PAGE_TelemtestEvent, NULL, "TeleGPS"},
    {PAGE_MainCfgInit, PAGE_MainCfgEvent, NULL, "MainConf"},
    {PAGE_AboutInit, NULL, NULL, "About"},

    //{PAGE_ScannerInit, PAGE_ScannerEvent, PAGE_ScannerExit},
};

static u8 page;
static u8 modal;
static u8 quick_page_enabled;
static struct buttonAction action;
static u8 (*ActionCB)(u32 button, u8 flags, void *data);
void PAGE_Init()
{
    page = 0;
    modal = 0;
    GUI_RemoveAllObjects();
    enter_cmd = NULL;
    exit_cmd = NULL;
    ActionCB = NULL;
    // For Devo10, there is no need to register and then unregister buttons in almost every page
    // since all buttons are needed in all pages, so we just register them in this common page
    BUTTON_RegisterCallback(&action,
          CHAN_ButtonMask(BUT_ENTER)
          | CHAN_ButtonMask(BUT_EXIT)
          | CHAN_ButtonMask(BUT_LEFT)
          | CHAN_ButtonMask(BUT_RIGHT)
          | CHAN_ButtonMask(BUT_UP)
          | CHAN_ButtonMask(BUT_DOWN),
          BUTTON_PRESS | BUTTON_LONGPRESS | BUTTON_RELEASE | BUTTON_PRIORITY, action_cb, NULL);
    PAGE_ChangeByName("MainPage", 0);

    labelDesc.font = DEFAULT_FONT.font;
    labelDesc.style = LABEL_LEFT;
    labelDesc.font_color = labelDesc.fill_color = labelDesc.outline_color = 0xffff; // not to draw box
}

void PAGE_ChangeByName(const char *pageName, s8 menuPage)
{
    if ( modal || GUI_IsModal())
        return;
    u8 p;
    u8 newpage = page;
    for(p = 0; p < sizeof(pages) / sizeof(struct page); p++) {
        if (strncmp(pages[p].pageName, pageName, PAGE_NAME_MAX) == 0) {
            newpage = p;
            break;
        }
    }
    if (pages[page].exit)
        pages[page].exit();
    page = newpage;
    if (pages[page].init == PAGE_MainInit)
        quick_page_enabled = 1;
    else if (pages[page].init == PAGE_MenuInit)
        quick_page_enabled = 0;
    PAGE_RemoveAllObjects();
    pages[page].init(menuPage);
}

void PAGE_Event()
{
    if (pages[page].event != NULL)
    {
        pages[page].event();
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

void PAGE_ShowHeader(const char *title)
{
    struct LabelDesc labelDesc;
    labelDesc.font = DEFAULT_FONT.font;
    labelDesc.font_color = 0xffff;
    labelDesc.style = LABEL_UNDERLINE;
    labelDesc.outline_color = 1;
    GUI_CreateLabelBox(0, 0, LCD_WIDTH, ITEM_HEIGHT, &labelDesc, NULL, NULL, title);
}

void PAGE_ShowHeaderWithHeight(const char *title, u8 font, u8 width, u8 height)
{
    struct LabelDesc labelDesc;
    labelDesc.font = font;
    labelDesc.font_color = 0xffff;
    labelDesc.style = LABEL_UNDERLINE;
    labelDesc.outline_color = 1;
    GUI_CreateLabelBox(0, 0, width, height, &labelDesc, NULL, NULL, title);
}

void PAGE_ShowHeader_ExitOnly(const char *title, void (*CallBack)(guiObject_t *obj, const void *data))
{
    (void)title;
    (void)CallBack;
    (void)enter_cmd;
    (void)enter_data;
    (void)exit_cmd;
    (void)exit_data;
    /*    enter_cmd = CallBack;
    enter_data = (void *)1;
    exit_cmd = CallBack;
    exit_data = (void *)0;
    GUI_CreateIcon(0, 0, &icons[ICON_EXIT], CallBack, (void *)0);
    GUI_CreateLabel(40, 10, NULL, TITLE_FONT, (void *)title); */
}

void PAGE_RemoveAllObjects()
{
    enter_cmd = NULL;
    exit_cmd = NULL;
    GUI_RemoveAllObjects();
}

void PAGE_SetActionCB(u8 (*callback)(u32 button, u8 flags, void *data))
{
    ActionCB = callback;
}

static u8 action_cb(u32 button, u8 flags, void *data)
{
    u8 result = 0;
    if (ActionCB != NULL)
        result = ActionCB(button, flags, data);
    if(! result && quick_page_enabled)
        result = PAGE_QuickPage(button, flags, data);
    return result;
}

void PAGE_NavigateItems(s8 direction, u8 view_id, u8 total_items, s8 *selectedIdx, s16 *view_origin_relativeY,
        guiObject_t *scroll_bar)
{
    if (total_items == 0)
        return;  // bug fix: avoid devided by 0
    guiObject_t *obj;
    for (u8 i = 0; i < (direction >0 ?direction:-direction); i++) {
        obj = GUI_GetSelected();
        if (direction > 0) {
            GUI_SetSelected((guiObject_t *)GUI_GetNextSelectable(obj));
        } else {
            GUI_SetSelected((guiObject_t *)GUI_GetPrevSelectable(obj));
        }
    }
    *selectedIdx += direction;
    *selectedIdx %= total_items;
    if (*selectedIdx == 0) {
        GUI_SetRelativeOrigin(view_id, 0, 0);
    } else {
        if (*selectedIdx < 0)
            *selectedIdx = total_items + *selectedIdx;
        obj = GUI_GetSelected();
        if (!GUI_IsObjectInsideCurrentView(view_id, obj))
            GUI_ScrollLogicalViewToObject(view_id, obj, direction);
    }
    *view_origin_relativeY = GUI_GetLogicalViewOriginRelativeY(view_id);
    if (scroll_bar != NULL)
        GUI_SetScrollbar(scroll_bar, *selectedIdx);
}

void PAGE_ChangeQuick(int dir)
{
    int quick = 0;
    for (int i = 0; i < 4; i++) {
        if(Model.pagecfg.quickpage[i] && Model.pagecfg.quickpage[i] == page) {
            quick = i+1;
            break;
        }
    }
    int increment = dir > 0 ? 1 : NUM_QUICKPAGES;
    while(1) {
       quick = (quick + increment) % 5;
       if (quick == 0 || Model.pagecfg.quickpage[quick-1])
           break;
    }
    if (quick == 0) {
        PAGE_ChangeByName("MainPage", 0);
    } else {
        PAGE_ChangeByName(pages[Model.pagecfg.quickpage[quick-1]].pageName, 0);
    }
}
int PAGE_QuickPage(u32 buttons, u8 flags, void *data)
{
    (void)data;

    if((flags & BUTTON_PRESS) && Model.pagecfg.quickbtn[0] &&
       CHAN_ButtonIsPressed(buttons, Model.pagecfg.quickbtn[0]))
    {
        PAGE_ChangeQuick(1);
        return 1;
    } else if ((flags & BUTTON_PRESS) && Model.pagecfg.quickbtn[1] &&
               CHAN_ButtonIsPressed(buttons, Model.pagecfg.quickbtn[1]))
    {
        PAGE_ChangeQuick(-1);
        return 1;
    }
    return 0;
}
const char *PAGE_GetName(int i)
{
    if(i == 0)
        return _tr("None");
    return _tr(pages[i].pageName);
}
int PAGE_GetNumPages()
{
    return sizeof(pages) / sizeof(struct page) - 1;
}
