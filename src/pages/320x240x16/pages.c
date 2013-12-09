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
static u8 page_change_cb(u32 buttons, u8 flags, void *data);
void PAGE_Exit();

#define PAGE_NAME_MAX 10
struct page {
    void (*init)(int i);
    void (*event)();
    void (*exit)();
    const char *name;
};


#define PAGEDEF(id, init, event, exit, name) {init, event, exit, name},
static const struct page pages[] = {
#include "pagelist.h"
};
#undef PAGEDEF
static u8 cur_section;
#include "../common/_pages.c"

struct page_group {
    u8 group;
    enum PageID id;
};

struct page_group groups[] = {
    {0, PAGEID_MAIN},
    {1, PAGEID_MIXER},
    {1, PAGEID_MODEL},
    {1, PAGEID_TIMER},
    {1, PAGEID_TELEMCFG},
    {1, PAGEID_TRIM},
#if DATALOG_ENABLED
    {1, PAGEID_DATALOG},
#endif
    {1, PAGEID_MAINCFG},
    {2, PAGEID_TXCFG},
    {2, PAGEID_TELEMMON},
    {2, PAGEID_CHANMON},
    {2, PAGEID_INPUTMON},
    {2, PAGEID_BTNMON},
#ifdef ENABLE_SCANNER
    {2, PAGEID_SCANNER},
#endif
    {2, PAGEID_USB},
#if HAS_STANDARD_GUI
    {0x81, PAGEID_MODELMENU},
#endif
    {255, PAGEID_SPLASH},
    {255, 0}
};


void PAGE_Init()
{
    cur_page = sizeof(pages) / sizeof(struct page) - 1;
    cur_section = 0;
    modal = 0;
    GUI_RemoveAllObjects();
    enter_cmd = NULL;
    exit_cmd = NULL;
    BUTTON_RegisterCallback(&button_action,
        CHAN_ButtonMask(BUT_ENTER) | CHAN_ButtonMask(BUT_EXIT)
        | CHAN_ButtonMask(BUT_RIGHT) | CHAN_ButtonMask(BUT_LEFT),
        BUTTON_PRESS | BUTTON_LONGPRESS, page_change_cb, NULL);
    PAGE_ChangeByID(PAGEID_SPLASH);
    //PAGE_ChangeByID(PAGEID_MAIN);
}

void PAGE_SetSection(u8 section)
{
    u8 p;
    u8 newpage = cur_page;

    if (section == SECTION_MODEL && Model.mixer_mode == MIXER_STANDARD)
        section = 0x80 | SECTION_MODEL;

    for(p = 0; groups[p].group != 255; p++) {
        if(groups[p].group == section) {
            newpage = p;
            break;
        }
    }
    if (newpage != cur_page) {
        cur_section = section;
        PAGE_ChangeByID(groups[newpage].id);
    }
}

void PAGE_Change(int dir)
{
    if ( modal || GUI_IsModal())
        return;
    if (Model.mixer_mode != 0 && (cur_page >= sizeof(groups) / sizeof(struct page_group) || groups[cur_page].group == 1)) {
        //Don't use left/right on model pages in standard mode
        return;
    }
    u8 nextpage = cur_page;
    if(dir > 0) {
        if (groups[nextpage+1].group == groups[cur_page].group) {
            nextpage++;
        } else {
            while(nextpage && groups[nextpage-1].group == groups[cur_page].group)
              nextpage--;
        } 
    } else if (dir < 0) {
        if (nextpage && groups[nextpage-1].group == groups[cur_page].group) {
            nextpage--;
        } else {
            while(groups[nextpage+1].group == groups[cur_page].group)
                nextpage++;
        }
    }
    if (cur_page == nextpage)
        return;
    PAGE_Exit();
    PAGE_ChangeByID(groups[nextpage].id);
}

void PAGE_ChangeByID(enum PageID id)
{
    if (cur_page != id) {
        PAGE_Exit();
        cur_page = id;
        PAGE_RemoveAllObjects();
        pages[cur_page].init(0);
    }
}

void changepage_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
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
}

void PAGE_RemoveHeader()
{
    if(OBJ_IS_USED(&gui->exitico))
        GUI_RemoveObj((guiObject_t *)&gui->exitico);
    if(OBJ_IS_USED(&gui->title))
        GUI_RemoveObj((guiObject_t *)&gui->title);
    if(OBJ_IS_USED(&gui->previco))
        GUI_RemoveObj((guiObject_t *)&gui->previco);
    if(OBJ_IS_USED(&gui->nextico))
        GUI_RemoveObj((guiObject_t *)&gui->nextico);
}

void PAGE_ShowHeader(const char *title)
{
    guiObject_t *obj;
    GUI_CreateIcon(&gui->exitico, 0, 0, &icons[ICON_EXIT], changepage_cb, (void *)0);
    if(title)
        GUI_CreateLabel(&gui->title, 40, 10, NULL, TITLE_FONT, (void *)title);
    obj = GUI_CreateIcon(&gui->previco, LCD_WIDTH-64, 0, &icons[ICON_PREVPAGE], changepage_cb, (void *)-1);
    GUI_SetSelectable(obj, 0);
    obj = GUI_CreateIcon(&gui->nextico, LCD_WIDTH-32, 0, &icons[ICON_NEXTPAGE], changepage_cb, (void *)1);
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
    GUI_CreateIcon(&gui->exitico, 0, 0, &icons[ICON_EXIT], CallBack, (void *)0);
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
    if(PAGE_QuickPage(buttons, flags, data))
        return 1;
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

const char *PAGE_GetName(int i)
{
    if(i == 0)
        return _tr("None");
    return _tr(pages[i].name);
}

int PAGE_GetStartPage()
{
    return 0;
}

int PAGE_GetNumPages()
{
    return sizeof(pages) / sizeof(struct page);
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
           || (Model.pagecfg2.quickpage[quick-1] && PAGE_IsValid(Model.pagecfg2.quickpage[quick-1])))
       {
           break;
       }
    }
    if (quick == 0) {
        PAGE_ChangeByID(PAGEID_MAIN);
    } else {
        PAGE_ChangeByID(Model.pagecfg2.quickpage[quick-1]);
    }
}

int PAGE_QuickPage(u32 buttons, u8 flags, void *data)
{
    (void)data;
    (void)flags;
    //static s8 press = 0;
    if(cur_section != 0)
        return 0;

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
