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
#include "pages.h"

static struct page_obj * const gui = &gui_objs.page;

static void (*enter_cmd)(guiObject_t *obj, const void *data);
static const void *enter_data;
static void (*exit_cmd)(guiObject_t *obj, const void *data);
static const void *exit_data;
static unsigned page_change_cb(u32 buttons, unsigned flags, void *data);
void PAGE_Exit();

static u8 quick_page_enabled = 1;
#include "../common/_pages.c"


void PAGE_Init()
{
    cur_page = 0;
    modal = 0;
    GUI_RemoveAllObjects();
    enter_cmd = NULL;
    exit_cmd = NULL;
    BUTTON_RegisterCallback(&button_action,
          CHAN_ButtonMask(BUT_ENTER)
          | CHAN_ButtonMask(BUT_EXIT)
          | CHAN_ButtonMask(BUT_LEFT)
          | CHAN_ButtonMask(BUT_RIGHT)
          | CHAN_ButtonMask(BUT_UP)
          | CHAN_ButtonMask(BUT_DOWN),
          BUTTON_PRESS | BUTTON_LONGPRESS | BUTTON_RELEASE | BUTTON_PRIORITY, page_change_cb, NULL);
    PAGE_ChangeByID(PAGEID_SPLASH, 0);
    //PAGE_ChangeByID(PAGEID_MAIN);
}

void PAGE_ChangeByID(enum PageID id, s8 menuPage)
{
    if ( modal || GUI_IsModal())
        return;
    if (pages[cur_page].exit)
        pages[cur_page].exit();
    cur_page = id;
    BUTTON_InterruptLongPress(); //Make sure button press is not passed to the new page
    if (pages[cur_page].init == PAGE_MainInit)
        quick_page_enabled = 1;
    else if (pages[cur_page].init == PAGE_MenuInit)
        quick_page_enabled = 0;
    PAGE_RemoveAllObjects();
    ActionCB = _action_cb;
    pages[cur_page].init(menuPage);
}

void changepage_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    PAGE_Pop();
#if 0
    if((long)data == 0) {
        PAGE_SetSection(SECTION_MAIN);
    } else if ((long)data == 1) {
        if (cur_section == 0)
            PAGE_ChangeQuick(1);
        else
            PAGE_Change(1);
    } else if ((long)data == -1) {
        if (cur_section == 0)
            PAGE_ChangeQuick(-1);
        else
            PAGE_Change(-1);
    }
#endif
}

void PAGE_RemoveHeader()
{
    if(OBJ_IS_USED(&gui->exitico))
        GUI_RemoveObj((guiObject_t *)&gui->exitico);
    if(OBJ_IS_USED(&gui->title))
        GUI_RemoveObj((guiObject_t *)&gui->title);
    //if(OBJ_IS_USED(&gui->previco))
    //    GUI_RemoveObj((guiObject_t *)&gui->previco);
    //if(OBJ_IS_USED(&gui->nextico))
    //    GUI_RemoveObj((guiObject_t *)&gui->nextico);
}

void PAGE_ShowHeader(const char *title)
{
    guiObject_t *obj;
    if (HAS_TOUCH) {
        obj = GUI_CreateIcon(&gui->exitico, 0, 0, &icons[ICON_EXIT], changepage_cb, (void *)0);
        GUI_SetSelectable(obj, 0);
    }
    if(title)
        GUI_CreateLabel(&gui->title, 40, 10, NULL, TITLE_FONT, (void *)title);
    //obj = GUI_CreateIcon(&gui->previco, LCD_WIDTH-64, 0, &icons[ICON_PREVPAGE], changepage_cb, (void *)-1);
    //GUI_SetSelectable(obj, 0);
    //obj = GUI_CreateIcon(&gui->nextico, LCD_WIDTH-32, 0, &icons[ICON_NEXTPAGE], changepage_cb, (void *)1);
    //GUI_SetSelectable(obj, 0);
    exit_cmd = changepage_cb;
    exit_data = NULL;
}


void PAGE_ShowHeader_ExitOnly(const char *title, void (*CallBack)(guiObject_t *obj, const void *data))
{
    guiObject_t *obj;
    enter_cmd = CallBack;
    enter_data = (void *)1;
    exit_cmd = CallBack;
    exit_data = (void *)0;
    if (HAS_TOUCH) {
        obj = GUI_CreateIcon(&gui->exitico, 0, 0, &icons[ICON_EXIT], CallBack, (void *)0);
        GUI_SetSelectable(obj, 0);
    }
    if(title)
        GUI_CreateLabel(&gui->title, 40, 10, NULL, TITLE_FONT, (void *)title);
}

void PAGE_ShowHeader_SetLabel(const char *(*label_cb)(guiObject_t *obj, const void *data), void *data)
{
    if(OBJ_IS_USED(&gui->title))
        GUI_RemoveObj((guiObject_t *)&gui->title);
    GUI_CreateLabelBox(&gui->title, 40, 10, 0, 0, &TITLE_FONT, label_cb, NULL, data);
}

static const char *okcancelstr_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    return data ? _tr("Ok") : _tr("Cancel");
}

unsigned page_change_cb(u32 buttons, unsigned flags, void *data)
{
    (void)data;
    (void)flags;
    if (PAGE_GetID() == PAGEID_TELEMMON) {
        if(CHAN_ButtonIsPressed(buttons, BUT_ENTER) || CHAN_ButtonIsPressed(buttons, BUT_EXIT))
            TELEMETRY_MuteAlarm();
    }
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
    if(PAGE_QuickPage(buttons, flags, data))
        return 1;
//    if(CHAN_ButtonIsPressed(buttons, BUT_RIGHT)) {
//        PAGE_Change(1);
//        return 1;
//    } else if(CHAN_ButtonIsPressed(buttons, BUT_LEFT)) {
//        PAGE_Change(-1);
//        return 1;
//    }
    return 0;
}

void PAGE_RemoveAllObjects()
{
    enter_cmd = NULL;
    exit_cmd = NULL;
    if(! GUI_IsEmpty()) {
        GUI_RemoveAllObjects();
        memset(&gui_objs, 0, sizeof(gui_objs));
        BUTTON_InterruptLongPress(); //Make sure button press is not passed to next page
    }
}

guiObject_t *PAGE_CreateCancelButton(u16 x, u16 y, void (*CallBack)(guiObject_t *obj, const void *data))
{
    exit_cmd = CallBack;
    exit_data = (void *)0;
    return GUI_CreateButton(&gui->previco, x, y, BUTTON_96, okcancelstr_cb, 0x0000, CallBack, (void *)0);
}
guiObject_t *PAGE_CreateOkButton(u16 x, u16 y, void (*CallBack)(guiObject_t *obj, const void *data))
{
    enter_cmd = CallBack;
    enter_data = (void *)1;
    return GUI_CreateButton(&gui->nextico, x, y, BUTTON_48, okcancelstr_cb, 0x0000, CallBack, (void *)1);
}

int PAGE_GetStartPage()
{
    return 0;
}

int PAGE_GetID()
{
    return cur_page;
}

void PAGE_ChangeQuick(int dir)
{
    int quick = 0;
    for (int i = 0; i < NUM_QUICKPAGES; i++) {
        if(Model.pagecfg2.quickpage[i] && Model.pagecfg2.quickpage[i] == cur_page) {
            quick = i+1;
            break;
        }
    }
    int increment = dir > 0 ? 1 : NUM_QUICKPAGES;
    while(1) {
       quick = (quick + increment) % 5;
       if (quick == 0
           || (Model.pagecfg2.quickpage[quick-1] && PAGE_IsValidQuickPage(Model.pagecfg2.quickpage[quick-1])))
       {
           break;
       }
    }
    if (quick == 0) {
        PAGE_ChangeByID(PAGEID_MAIN, 0);
    } else {
        PAGE_ChangeByID(Model.pagecfg2.quickpage[quick-1], 0);
    }
}

int PAGE_QuickPage(u32 buttons, u8 flags, void *data)
{
    (void)data;
    (void)flags;
    //static s8 press = 0;
    //if(cur_section != 0)
    //    return 0;

/*    if (press) {
        if (flags & BUTTON_RELEASE) {
            PAGE_ChangeQuick(press);
            press = 0;
        }
        return 1;
    }
*/
    int i;
    for(i = 0; i < NUM_QUICKPAGES; i++)
        if(Model.pagecfg2.quickpage[i])
            break;
    if(i == NUM_QUICKPAGES)
        return 0;
    if(CHAN_ButtonIsPressed(buttons, BUT_RIGHT)) {
        //press = 1;
        PAGE_ChangeQuick(1);
        return 1;
    } else if (CHAN_ButtonIsPressed(buttons, BUT_LEFT)) {
        //press = -1;
        PAGE_ChangeQuick(-1);
        return 1;
    }
    return 0;
}
