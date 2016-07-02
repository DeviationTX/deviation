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

struct LabelDesc labelDesc; // create a style-customizable font so that it can be shared for all pages

static unsigned action_cb(u32 button, unsigned flags, void *data);

#include "../common/_pages.c"

static u8 quick_page_enabled;
static u16 *current_selected;
static guiScrollable_t *page_scrollable;
void PAGE_Init()
{
    cur_page = 0;
    modal = 0;
    page_scrollable = NULL;
    GUI_RemoveAllObjects();
    // For Devo10, there is no need to register and then unregister buttons in almost every page
    // since all buttons are needed in all pages, so we just register them in this common page
    BUTTON_RegisterCallback(&button_action,
          CHAN_ButtonMask(BUT_ENTER)
          | CHAN_ButtonMask(BUT_EXIT)
          | CHAN_ButtonMask(BUT_LEFT)
          | CHAN_ButtonMask(BUT_RIGHT)
          | CHAN_ButtonMask(BUT_UP)
          | CHAN_ButtonMask(BUT_DOWN),
          BUTTON_PRESS | BUTTON_LONGPRESS | BUTTON_RELEASE | BUTTON_PRIORITY, action_cb, NULL);
    //PAGE_ChangeByID(PAGEID_MAIN, 0);
    PAGE_ChangeByID(PAGEID_SPLASH, 0);

    labelDesc.font = DEFAULT_FONT.font;
    labelDesc.style = LABEL_LEFT;
    labelDesc.font_color = labelDesc.fill_color = labelDesc.outline_color = 0xffff; // not to draw box
}

void PAGE_ChangeByID(enum PageID id, s8 menuPage)
{
    if ( modal || GUI_IsModal())
        return;
    PAGE_SaveCurrentPos();
    page_scrollable = NULL;
    if (pages[cur_page].exit) {
        pages[cur_page].exit();
    }
    GUI_SelectionNotify(NULL);
    cur_page = id;
    BUTTON_InterruptLongPress(); //Make sure button press is not passed to the new page
    if (pages[cur_page].init == PAGE_MainInit)
        quick_page_enabled = 1;
    else if (pages[cur_page].init == PAGE_MenuInit)
        quick_page_enabled = 0;
    PAGE_RemoveAllObjects();
    ActionCB = default_button_action_cb;
    pages[cur_page].init(menuPage);
    if (page_scrollable) {
        GUI_SetSelected(GUI_ShowScrollableRowOffset(page_scrollable, *current_selected));
    }
}

static guiLabel_t headerLabel;
void PAGE_ShowHeader(const char *title)
{
    struct LabelDesc labelDesc;
    labelDesc.font = DEFAULT_FONT.font;
    labelDesc.font_color = 0xffff;
    labelDesc.style = LABEL_UNDERLINE;
    labelDesc.outline_color = 1;
    labelDesc.fill_color = 0;
    GUI_CreateLabelBox(&headerLabel, 0, 0, LCD_WIDTH, HEADER_HEIGHT, &labelDesc, NULL, NULL, title);
}

void PAGE_ShowHeaderWithHeight(const char *title, u8 font, u8 width, u8 height)
{
    struct LabelDesc labelDesc;
    labelDesc.font = font;
    labelDesc.font_color = 0xffff;
    labelDesc.style = LABEL_UNDERLINE;
    labelDesc.outline_color = 1;
    labelDesc.fill_color = 0;
    GUI_CreateLabelBox(&headerLabel, 0, 0, width, height, &labelDesc, NULL, NULL, title);
}

void PAGE_ShowHeader_SetLabel(const char *(*label_cb)(guiObject_t *obj, const void *data), void *data)
{
    (void)label_cb;
    (void)data;
}
void PAGE_RemoveHeader()
{}

void PAGE_RemoveAllObjects()
{
    if(! GUI_IsEmpty()) {
        GUI_RemoveAllObjects();
        memset(&gui_objs, 0, sizeof(gui_objs));
        BUTTON_InterruptLongPress(); //Make sure button press is not passed to the new page
    }
}

static unsigned action_cb(u32 button, unsigned flags, void *data)
{
    unsigned result = 0;
    if(GUI_IsModal())  //Disable control when a dialog is shown
        return 0;
    if(! result && quick_page_enabled)  // let the quickpage over other pages
        result = PAGE_QuickPage(button, flags, data);
    if (!result && ActionCB != NULL)
        result = ActionCB(button, flags, data);
    return result;
}

void PAGE_ChangeQuick(int dir)
{
    int quick = 0;
    for (int i = 0; i < 4; i++) {
        if(Model.pagecfg2.quickpage[i] > 1 && Model.pagecfg2.quickpage[i] == cur_page) {
            quick = i+1;
            break;
        }
    }
    int increment = dir > 0 ? 1 : NUM_QUICKPAGES;
    while(1) {
       quick = (quick + increment) % 5;
       if (quick == 0 || Model.pagecfg2.quickpage[quick-1] > 1)
           break;
    }
    if (quick == 0) {
        PAGE_ChangeByID(PAGEID_MAIN, 0);
    } else if (Model.pagecfg2.quickpage[quick-1] == 1) { // bug fix: main menu should not be in quick page
    } else {
        PAGE_ChangeByID(Model.pagecfg2.quickpage[quick-1], 0);
    }
}
int PAGE_QuickPage(u32 buttons, u8 flags, void *data)
{
    (void)data;

    if((flags & BUTTON_LONGPRESS) && CHAN_ButtonIsPressed(buttons, BUT_UP))
    {
        PAGE_ChangeQuick(1);
        return 1;
    } else if ((flags & BUTTON_LONGPRESS) && CHAN_ButtonIsPressed(buttons, BUT_DOWN))
    {
        PAGE_ChangeQuick(-1);
        return 1;
    }
    return 0;
}

int PAGE_GetStartPage()
{
    return 1; // main menu shouldn't be put to quick page
}

int PAGE_GetID()
{
    return cur_page;
}

void PAGE_SaveMixerSetup(struct mixer_page * const mp)
{
    MIXER_SetTemplate(mp->channel, mp->cur_template);
    MIXER_SetMixers(mp->mixer, mp->num_mixers);
    MUSIC_Play(MUSIC_SAVING); // no saving tone in the sound.ini
    BUTTON_InterruptLongPress();
}

