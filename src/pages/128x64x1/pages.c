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
    {PAGE_CalibrateInit, PAGE_CalibrateEvent, PAGE_CalibrateExit, "Calibrate"},
    {PAGE_SingleItemConfigInit, NULL, PAGE_SingleItemConfigExit, "SingItem"},
    {PAGE_ProtocolSelectInit, NULL, PAGE_ProtocolSelectExit, "ProtoSele"},
/*    {PAGE_MixerInit, PAGE_MixerEvent, NULL},
    {PAGE_ModelInit, PAGE_ModelEvent, NULL},
    {PAGE_TimerInit, PAGE_TimerEvent, NULL},
    {PAGE_TrimInit, PAGE_TrimEvent, NULL},
    {PAGE_MainCfgInit, PAGE_MainCfgEvent, NULL},
    {NULL, NULL, NULL},
    {PAGE_ChantestInit, PAGE_ChantestEvent, PAGE_ChantestExit},
    {PAGE_InputtestInit, PAGE_ChantestEvent, PAGE_ChantestExit},
    {PAGE_ButtontestInit, PAGE_ChantestEvent, PAGE_ChantestExit},
    {PAGE_ScannerInit, PAGE_ScannerEvent, PAGE_ScannerExit},
    //{PAGE_TestInit, PAGE_TestEvent, NULL},
    {PAGE_USBInit, PAGE_USBEvent, PAGE_USBExit},
*/
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
    labelDesc.style = UNDERLINE;
    labelDesc.outline_color = 1;
	GUI_CreateLabelBox(0, 0, LCD_WIDTH, 0, &labelDesc, NULL, NULL, title);
	GUI_CreateRect(0, MENU_ITEM_HEIGHT, LCD_WIDTH, 1, &labelDesc); // draw a line only
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

