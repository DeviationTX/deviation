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

//static buttonAction_t button_action;

static void (*enter_cmd)(guiObject_t *obj, const void *data);
static const void *enter_data;
static void (*exit_cmd)(guiObject_t *obj, const void *data);
static const void *exit_data;
static u8 action_cb(u32 button, u8 flags, void *data);

struct page {
    void (*init)(int i);
    void (*event)();
    void (*exit)();
    const char pageName[PAGE_NAME_MAX+1];
};

struct pagemem pagemem;

static const struct page pages[] = {
    {PAGE_MainInit, PAGE_MainEvent, PAGE_MainExit, "MainPage"},  // Note: the page name lenth should not exceed 10 chars
    {PAGE_MainMenuInit, NULL, PAGE_MainMenuExit, "MainMenu"},
    {PAGE_SubMenuInit, NULL, PAGE_SubMenuExit, "SubMenu" },
    {PAGE_ChantestInit, PAGE_ChantestEvent, PAGE_ChantestExit, "Monitor"},
    {PAGE_SingleItemConfigInit, NULL, PAGE_SingleItemConfigExit, "SingItem"},
    {PAGE_MultiItemsConfigInit, NULL, PAGE_MultiItemsConfigExit, "MulItems"},
    {PAGE_MixerInit, PAGE_MixerEvent, NULL, "Mixer"},
    {PAGE_TxConfigureInit, PAGE_TxConfigureEvent, NULL, "TxConfig"},
    {PAGE_ModelInit, PAGE_ModelEvent, NULL, "ModelCon"},
    {PAGE_TrimInit, PAGE_TrimEvent, NULL, "Trim"},
    {PAGE_TimerInit, PAGE_TimerEvent, NULL, "Timer"},

    /*
    {PAGE_MainCfgInit, PAGE_MainCfgEvent, NULL},
    {NULL, NULL, NULL},
    {PAGE_ScannerInit, PAGE_ScannerEvent, PAGE_ScannerExit},
    //{PAGE_TestInit, PAGE_TestEvent, NULL},
    {PAGE_USBInit, PAGE_USBEvent, PAGE_USBExit},
*/
};

static u8 page;
static u8 modal;
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
    pages[page].init(0);
}

void PAGE_ChangeByName(const char *pageName, u8 menuPage)
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
    if (newpage != page) {
        if (pages[page].exit)
            pages[page].exit();
        page = newpage;
        PAGE_RemoveAllObjects();
        pages[page].init(menuPage);
    }
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
    GUI_CreateLabelBox(0, 0, LCD_WIDTH, MENU_ITEM_HEIGHT, &labelDesc, NULL, NULL, title);
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
    return data ? _tr("OK") : _tr("Cancel");
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

void PAGE_SetActionCB(u8 (*callback)(u32 button, u8 flags, void *data))
{
    ActionCB = callback;
}

static u8 action_cb(u32 button, u8 flags, void *data)
{
    u8 result = 0;
    if (ActionCB != NULL)
        result = ActionCB(button, flags, data);
    return result;
}

void PAGE_NavigateItems(s8 direction, u8 view_id, u8 total_items, s8 *selectedIdx, s16 *view_origin_relativeY,
        guiObject_t *scroll_bar)
{
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
    if (selectedIdx == 0) {
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
