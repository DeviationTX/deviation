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
#define gui3 (&gui_objs.u.tx.u.g3)
#if LCD_WIDTH != 480
    #define gui1 (&gui_objs.u.tx.u.g1)
    #define gui2 (&gui_objs.u.tx.u.g2)
#else
    #define gui1 gui3
    #define gui2 gui3
#endif
#define MIN_BATTERY_ALARM_STEP 50

u8 page_num;
guiObject_t *firstObj;

void PAGE_ChangeByName(const char *pageName, u8 menuPage)
{   // dummy method for devo8, only used in devo10
    (void)pageName;
    (void)menuPage;
}
#include "../common/_tx_configure.c"
#if LCD_WIDTH != 480
    #define MAX_PAGE 2
#else
    #define MAX_PAGE 0
#endif
static void _show_page();

static int scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data)
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
    #define COL1 16
    #define COL2 (COL1+106)
    #if LCD_WIDTH == 480
        #define COL3         (LCD_WIDTH-16-106-96) // border + label + button
        #define COL4         (LCD_WIDTH-16-96)    // border + button
        #define BUTTON_WIDE  BUTTON_96x16
        #define BUTTON_TEST  BUTTON_48x16
        #define BUTTON_TOUCH BUTTON_TEST
        #define COL2TEST     (COL2+48)
        #define ADDSPACE     0
        #define ADDROW       0
    #else
        #define COL3         COL1
        #define COL4         COL2
        #define BUTTON_WIDE  BUTTON_96
        #define BUTTON_TEST  BUTTON_48
        #define BUTTON_TOUCH BUTTON_WIDE
        #define COL2TEST     (COL2+100)
        #define ADDSPACE     15
        #define ADDROW       6
    #endif
    guiObject_t *obj;
    u8 space = 19;
    int row = 40;
    int col1 = COL1;
    int col2 = COL2;
    if (page_num == 0 || LCD_WIDTH == 480) {
        obj = GUI_CreateLabelBox(&gui1->head1_1, col1, row, 0, 0, &SECTION_FONT, NULL, NULL, _tr("Generic settings"));
        if (firstObj == NULL) firstObj = obj;
        row += space;
        if (row+12 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
        GUI_CreateLabelBox(&gui1->langlbl, col1, row+ADDROW, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Language"));
        GUI_CreateButton(&gui1->lang, col2, row, BUTTON_WIDE, langstr_cb, 0x0000, lang_select_cb, NULL);
        row += space + ADDSPACE ;
        if (row+12 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
        GUI_CreateLabelBox(&gui1->modelbl, col1, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Stick mode"));
        GUI_CreateTextSelect(&gui1->mode, col2, row, TEXTSELECT_96, NULL, modeselect_cb, NULL);
        row += space + ADDSPACE;
        if (row+12 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
        GUI_CreateLabelBox(&gui1->touchlbl, col1, row+ADDROW, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Touch screen"));
        GUI_CreateButton(&gui1->touchcalib, col2, row, BUTTON_TOUCH, calibratestr_cb, 0x0000, press_cb, (void *)CALIB_TOUCH);
        GUI_CreateButton(&gui1->touchtest, COL2TEST, row, BUTTON_TEST, calibratestr_cb, 0x0000, press_cb, (void *)CALIB_TOUCH_TEST);
        row += space + ADDSPACE;
        if (row+12 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
        GUI_CreateLabelBox(&gui1->sticklbl, col1, row+ADDROW, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Sticks"));
        GUI_CreateButton(&gui1->stickcalib, col2, row, BUTTON_WIDE, calibratestr_cb, 0x0000, press_cb, (void *)CALIB_STICK);
        row += space + (LCD_WIDTH == 320 ? 8 : 16); // for nicer look
        if (row+12 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
    }
    if (page_num == 1 || LCD_WIDTH == 480) {
        obj = GUI_CreateLabelBox(&gui2->head2_1, col1, row, 0, 0, &SECTION_FONT, NULL, NULL, _tr("Buzzer settings"));
        if (firstObj == NULL) firstObj = obj;
        row += space;
        if (row+12 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
        GUI_CreateLabelBox(&gui2->power_alarmlbl, col1, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Power On alarm"));
        GUI_CreateTextSelect(&gui2->power_alarm, col2, row, TEXTSELECT_96, NULL, poweralarm_select_cb, NULL);
        row += space;
        if (row+12 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
        GUI_CreateLabelBox(&gui2->battalrmlbl, col1, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Battery alarm"));
        GUI_CreateTextSelect(&gui2->battalrm, col2, row, TEXTSELECT_96, NULL, batalarm_select_cb, NULL);
        row += space;
        if (row+12 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
        GUI_CreateLabelBox(&gui2->battalrmintvllbl, col1, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Alarm intvl"));
        GUI_CreateTextSelect(&gui2->battalrmintvl, col2, row, TEXTSELECT_96, NULL, batalarmwarn_select_cb, NULL);
        row += space;
        if (row+12 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
        GUI_CreateLabelBox(&gui2->buzzlbl, col1, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Buzz volume"));
        GUI_CreateTextSelect(&gui2->buzz, col2, row, TEXTSELECT_96, NULL, _buzz_vol_cb, (void *)&Transmitter.volume);
        row += space;
        if (row+12 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
        GUI_CreateLabelBox(&gui2->musicshutdbl, col1, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Power-down alert"));
        GUI_CreateTextSelect(&gui2->music_shutdown, col2, row, TEXTSELECT_96, NULL, _music_shutdown_cb, (void *)&Transmitter.music_shutdown);
        row += space + 8;
        if (row+12 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
        GUI_CreateLabelBox(&gui2->head2_2, col1, row, 0, 0, &SECTION_FONT, NULL, NULL, _tr("LCD settings"));
        row += space;
        if (row+8 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
        GUI_CreateLabelBox(&gui2->backlightlbl, col1, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Backlight"));
        GUI_CreateTextSelect(&gui2->backlight, col2, row, TEXTSELECT_96, NULL, brightness_select_cb, NULL);
        row += space;
        if (row+8 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
        GUI_CreateLabelBox(&gui2->dimtimelbl, col1, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Dimmer time"));
        GUI_CreateTextSelect(&gui2->dimtime, col2, row, TEXTSELECT_96, NULL, auto_dimmer_time_cb, NULL);
        row += space;
        if (row+8 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
        GUI_CreateLabelBox(&gui2->dimtgtlbl, col1, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Dimmer target"));
        GUI_CreateTextSelect(&gui2->dimtgt, col2, row, TEXTSELECT_96, NULL, common_select_cb,
                (void *)&Transmitter.auto_dimmer.backlight_dim_value);
        row += space + 8;
        if (row+8 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
    }
    if (page_num == 2 || LCD_WIDTH == 480) {
        obj = GUI_CreateLabelBox(&gui3->head3_1, col1, row, 0, 0, &SECTION_FONT, NULL, NULL, _tr("Timer settings"));
        if (firstObj == NULL) firstObj = obj;
        row += space;
        if (row+8 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
        GUI_CreateLabelBox(&gui3->prealertlbl, col1, row, 0, 0, &DEFAULT_FONT, NULL, NULL,  _tr("Prealert time"));
        GUI_CreateTextSelect(&gui3->prealert, col2, row, TEXTSELECT_96,NULL, prealert_time_cb, (void *)0L);
        row += space;
        if (row+8 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
        GUI_CreateLabelBox(&gui3->preintvllbl, col1, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Prealert intvl"));
        GUI_CreateTextSelect(&gui3->preintvl, col2, row, TEXTSELECT_96, NULL, timer_interval_cb,
                &Transmitter.countdown_timer_settings.prealert_interval);
        row += space;
        if (row+8 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
        GUI_CreateLabelBox(&gui3->timeuplbl, col1, row, 0, 0, &DEFAULT_FONT, NULL, NULL,_tr("Timeup intvl"));
        GUI_CreateTextSelect(&gui3->timeup, col2, row, TEXTSELECT_96, NULL, timer_interval_cb,
                &Transmitter.countdown_timer_settings.timeup_interval);
        row += space + 8;
        if (row+8 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
        GUI_CreateLabelBox(&gui3->head3_2, col1, row, 0, 0, &SECTION_FONT, NULL, NULL, _tr("Telemetry settings"));
        row += space;
        if (row+8 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
        GUI_CreateLabelBox(&gui3->templbl, col1, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Temperature"));
        GUI_CreateTextSelect(&gui3->temp, col2, row, TEXTSELECT_96, NULL, units_cb, (void *)1L);
        row += space;
        if (row+8 >= LCD_HEIGHT) { row = 40; col1 = COL3; col2 = COL4; }
        GUI_CreateLabelBox(&gui3->lengthlbl, col1, row, 0, 0, &DEFAULT_FONT, NULL, NULL, _tr("Length"));
        GUI_CreateTextSelect(&gui3->length, col2, row, TEXTSELECT_96, NULL, units_cb, (void *)0L);
    }
}

void PAGE_TxConfigureInit(int page)
{
    (void)page;
    cp->enable = CALIB_NONE;
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    PAGE_ShowHeader(PAGE_GetName(PAGEID_TXCFG));
    GUI_CreateScrollbar(&gui->scrollbar, LCD_WIDTH-16, 32, LCD_HEIGHT-32, MAX_PAGE+1, NULL, scroll_cb, NULL);
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
