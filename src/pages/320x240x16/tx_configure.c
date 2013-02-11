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
#include "gui/gui.h"
#include "config/tx.h"
#include "config/model.h"


#define gui (&gui_objs.u.tx)
#define gui1 (&gui_objs.u.tx.u.g1)
#define gui2 (&gui_objs.u.tx.u.g2)
#define gui3 (&gui_objs.u.tx.u.g3)

#define MIN_BATTERY_ALARM_STEP 50

u8 page_num;
guiObject_t *firstObj;

void PAGE_ChangeByName(const char *pageName, u8 menuPage)
{   // dummy method for devo8, only used in devo10
    (void)pageName;
    (void)menuPage;
}
#include "../common/_tx_configure.c"
#define MAX_PAGE 2
static void _show_page();

static u8 scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data)
{
    (void)pos;
    (void)parent;
    (void)data;
    s8 newpos = (s8)page_num + (direction > 0 ? 1 : -1);
    if (newpos < 0)
        newpos = 0;
    else if (newpos > MAX_PAGE)
        newpos = MAX_PAGE;
    if (newpos != page_num) {
        page_num = newpos;
        _show_page();
    }
    return page_num;
}

static void _show_page()
{
    if (firstObj) {
        GUI_RemoveHierObjects(firstObj);
        firstObj = NULL;
    }
    u8 space = 19;
    u8 row = 40;
    if (page_num == 0) {
        firstObj = GUI_CreateLabelBox(&gui1->head1, 16, row, 0, 0,
                   &SECTION_FONT, NULL, NULL, _tr("Generic settings"));
        row += space;
        GUI_CreateLabelBox(&gui1->langlbl, 16, row+6, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Language:"));
        GUI_CreateButton(&gui1->lang, 112, row, BUTTON_96, langstr_cb, 0x0000, lang_select_cb, NULL);
        row += space + 15 ;
        GUI_CreateLabelBox(&gui1->modelbl, 16, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Stick mode:"));
        GUI_CreateTextSelect(&gui1->mode, 112, row, TEXTSELECT_96, NULL, modeselect_cb, NULL);
        row += space + 15;
        GUI_CreateLabelBox(&gui1->touchlbl, 16, row+6, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Touch screen:"));
        GUI_CreateButton(&gui1->touchcalib, 112, row, BUTTON_96, calibratestr_cb, 0x0000, press_cb, (void *)CALIB_TOUCH);
        GUI_CreateButton(&gui1->touchtest, 216, row, BUTTON_48, calibratestr_cb, 0x0000, press_cb, (void *)CALIB_TOUCH_TEST);
        row += space + 15;
        GUI_CreateLabelBox(&gui1->sticklbl, 16, row+6, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Sticks:"));
        GUI_CreateButton(&gui1->stickcalib, 112, row, BUTTON_96, calibratestr_cb, 0x0000, press_cb, (void *)CALIB_STICK);
    } else if (page_num == 1) {
        firstObj = GUI_CreateLabelBox(&gui2->head1, 16, row, 0, 0, &SECTION_FONT, NULL, NULL, _tr("Buzzer settings"));
        row += space;
        GUI_CreateLabelBox(&gui2->battalrmlbl, 16, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Battery alarm:"));
        GUI_CreateTextSelect(&gui2->battalrm, 112, row, TEXTSELECT_96, NULL, batalarm_select_cb, NULL);
        row += space;
        GUI_CreateLabelBox(&gui2->battalrmintvllbl, 16, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Alarm intvl:"));
        GUI_CreateTextSelect(&gui2->battalrmintvl, 112, row, TEXTSELECT_96, NULL, batalarmwarn_select_cb, NULL);
        row += space;
        GUI_CreateLabelBox(&gui2->buzzlbl, 16, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Buzz volume:"));
        GUI_CreateTextSelect(&gui2->buzz, 112, row, TEXTSELECT_96, NULL, _buzz_vol_cb, (void *)&Transmitter.volume);
        row += space;
        GUI_CreateLabelBox(&gui2->musicshutdbl, 16, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("PowerOff alert"));
        GUI_CreateTextSelect(&gui2->music_shutdown, 112, row, TEXTSELECT_96, NULL, _music_shutdown_cb, (void *)&Transmitter.music_shutdown);
        row += space + 8;
        GUI_CreateLabelBox(&gui2->head2, 16, row, 0, 0, &SECTION_FONT, NULL, NULL, _tr("LCD settings"));
        row += space;
        GUI_CreateLabelBox(&gui2->backlightlbl, 16, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Backlight:"));
        GUI_CreateTextSelect(&gui2->backlight, 112, row, TEXTSELECT_96, NULL, brightness_select_cb, NULL);
        row += space;
        GUI_CreateLabelBox(&gui2->dimtimelbl, 16, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Dimmer time:"));
        GUI_CreateTextSelect(&gui2->dimtime, 112, row, TEXTSELECT_96, NULL, auto_dimmer_time_cb, NULL);
        row += space;
        GUI_CreateLabelBox(&gui2->dimtgtlbl, 16, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Dimmer target:"));
        GUI_CreateTextSelect(&gui2->dimtgt, 112, row, TEXTSELECT_96, NULL, common_select_cb,
                (void *)&Transmitter.auto_dimmer.backlight_dim_value);
        row += space + 8;
    } else if (page_num == 2) {
        firstObj = GUI_CreateLabelBox(&gui3->head1, 16, row, 0, 0, &SECTION_FONT, NULL, NULL, _tr("Timer settings"));
        row += space;
        GUI_CreateLabelBox(&gui3->prealertlbl, 16, row, 0, 0, &DEFAULT_FONT, NULL, NULL,  _tr("Prealert time:"));
        GUI_CreateTextSelect(&gui3->prealert, 130, row, TEXTSELECT_96,NULL, prealert_time_cb, (void *)0L);
        row += space;
        GUI_CreateLabelBox(&gui3->preintvllbl, 16, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Prealert intvl:"));
        GUI_CreateTextSelect(&gui3->preintvl, 130, row, TEXTSELECT_96, NULL, timer_interval_cb,
                &Transmitter.countdown_timer_settings.prealert_interval);
        row += space;
        GUI_CreateLabelBox(&gui3->timeuplbl, 16, row, 0, 0, &DEFAULT_FONT, NULL, NULL,_tr("Timeup intvl:"));
        GUI_CreateTextSelect(&gui3->timeup, 130, row, TEXTSELECT_96, NULL, timer_interval_cb,
                &Transmitter.countdown_timer_settings.timeup_interval);
        row += space + 8;
        GUI_CreateLabelBox(&gui3->head2, 16, row, 0, 0, &SECTION_FONT, NULL, NULL, _tr("Telemetry settings"));
        row += space;
        GUI_CreateLabelBox(&gui3->templbl, 16, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Temperature:"));
        GUI_CreateTextSelect(&gui3->temp, 130, row, TEXTSELECT_96, NULL, units_cb, (void *)1L);
        row += space;
        GUI_CreateLabelBox(&gui3->lengthlbl, 16, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Length:"));
        GUI_CreateTextSelect(&gui3->length, 130, row, TEXTSELECT_96, NULL, units_cb, (void *)0L);
    }
}

void PAGE_TxConfigureInit(int page)
{
    (void)page;
    cp->enable = CALIB_NONE;
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(PAGE_GetName(PAGEID_TXCFG));
    GUI_CreateScrollbar(&gui->scrollbar, 304, 32, 208, MAX_PAGE+1, NULL, scroll_cb, NULL);
    firstObj = NULL;
    page_num = 0;
    _show_page();
}

static inline guiObject_t *_get_obj(int idx, int objid)
{
    (void)idx;
    (void)objid;
    switch (idx) {
        case ITEM_DIMVAL: return (guiObject_t *)&gui2->dimtgt;
        default: return NULL;
    }
}
