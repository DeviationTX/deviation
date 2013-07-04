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
#include "telemetry.h"
#include "gui/gui.h"

#include "../common/_telemtest_page.c"

static const u8 row_height = 20;
struct telempos {
    u16 x;
    u16 y;
    u8 width;
    u8 height;
};

struct telem_layout {
    struct telempos label;
    struct telempos value;
    u8 source;
};

const struct telem_layout devo8_layout[] = {
          {{10, 40, 40, 16}, {60, 40, 40, 16}, TELEM_DEVO_TEMP1},
          {{10, 60, 40, 16}, {60, 60, 40, 16}, TELEM_DEVO_TEMP2},
          {{10, 80, 40, 16}, {60, 80, 40, 16}, TELEM_DEVO_TEMP3},
          {{10,100, 40, 16}, {60,100, 40, 16}, TELEM_DEVO_TEMP4},
          {{110, 40, 40, 16}, {155, 40, 40, 16}, TELEM_DEVO_VOLT1},
          {{110, 60, 40, 16}, {155, 60, 40, 16}, TELEM_DEVO_VOLT2},
          {{110, 80, 40, 16}, {155, 80, 40, 16}, TELEM_DEVO_VOLT3},
          {{210, 40, 40, 16}, {255, 40, 40, 16}, TELEM_DEVO_RPM1},
          {{210, 60, 40, 16}, {255, 60, 40, 16}, TELEM_DEVO_RPM2},
          {{20, 140, 60, 16}, {100, 140, 200, 16}, TELEM_GPS_LAT},
          {{20, 160, 60, 16}, {100, 160, 200, 16}, TELEM_GPS_LONG},
          {{20, 180, 60, 16}, {100, 180, 200, 16}, TELEM_GPS_ALT},
          {{20, 200, 60, 16}, {100, 200, 200, 16}, TELEM_GPS_SPEED},
          {{20, 220, 60, 16}, {100, 220, 200, 16}, TELEM_GPS_TIME},
          {{0, 0, 0, 0}, {0, 0, 0, 0}, 0},
};
const struct telem_layout dsm_layout[] = {
          {{10, 40, 40, 16}, {60, 40, 40, 16}, TELEM_DSM_FLOG_FADESA},
          {{10, 60, 40, 16}, {60, 60, 40, 16}, TELEM_DSM_FLOG_FADESB},
          {{10, 80, 40, 16}, {60, 80, 40, 16}, TELEM_DSM_FLOG_FADESL},
          {{10,100, 40, 16}, {60,100, 40, 16}, TELEM_DSM_FLOG_FADESR},
          {{110, 40, 40, 16}, {155, 40, 40, 16}, TELEM_DSM_FLOG_FRAMELOSS},
          {{110, 60, 40, 16}, {155, 60, 40, 16}, TELEM_DSM_FLOG_HOLDS},
          {{210, 40, 40, 16}, {255, 40, 40, 16}, TELEM_DSM_FLOG_VOLT1},
          {{210, 60, 40, 16}, {255, 60, 40, 16}, TELEM_DSM_FLOG_VOLT2},
          {{210, 80, 40, 16}, {255, 80, 40, 16}, TELEM_DSM_FLOG_RPM1},
          {{210,100, 40, 16}, {255,100, 40, 16}, TELEM_DSM_FLOG_TEMP1},
          {{20, 140, 60, 16}, {100, 140, 200, 16}, TELEM_GPS_LAT},
          {{20, 160, 60, 16}, {100, 160, 200, 16}, TELEM_GPS_LONG},
          {{20, 180, 60, 16}, {100, 180, 200, 16}, TELEM_GPS_ALT},
          {{20, 200, 60, 16}, {100, 200, 200, 16}, TELEM_GPS_SPEED},
          {{20, 220, 60, 16}, {100, 220, 200, 16}, TELEM_GPS_TIME},
          {{0, 0, 0, 0}, {0, 0, 0, 0}, 0},
};
static void show_page(const struct telem_layout *layout)
{
    int i = 0;
    for(const struct telem_layout *ptr = layout; ptr->source; ptr++) {
        GUI_CreateLabelBox(&gui->label[i], ptr->label.x, ptr->label.y,
                           ptr->label.width, ptr->label.height, &TELEM_TXT_FONT,
                           label_cb, NULL, (void *)(long)ptr->source);
        GUI_CreateLabelBox(&gui->value[i], ptr->value.x, ptr->value.y,
                           ptr->value.width, ptr->value.height, &TELEM_ERR_FONT,
                           telem_cb, NULL, (void *)(long)ptr->source);
        i++;
    }
    tp.telem = Telemetry;
    //memset(tp.telem.time, 0, sizeof(tp.telem.time));
}

void PAGE_TelemtestInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_ShowHeader(PAGE_GetName(PAGEID_TELEMMON));
    if (telem_state_check() == 0) {
        GUI_CreateLabelBox(&gui->msg, 20, 80, 280, 100, &NARROW_FONT, NULL, NULL, tp.str);
        return;
    }
    show_page(TELEMETRY_Type() == TELEM_DEVO ? devo8_layout : dsm_layout);
}

void PAGE_TelemtestModal(void(*return_page)(int page), int page)
{
    PAGE_SetModal(1);
    tp.return_page = return_page;
    tp.return_val = page;
    PAGE_RemoveAllObjects();

    PAGE_ShowHeader_ExitOnly(PAGE_GetName(PAGEID_TELEMMON), okcancel_cb);
    if (telem_state_check() == 0) {
        GUI_CreateLabelBox(&gui->msg, 20, 80, 280, 100, &NARROW_FONT, NULL, NULL, tp.str);
        return;
    }

    show_page(TELEMETRY_Type() == TELEM_DEVO ? devo8_layout : dsm_layout);
}
void PAGE_TelemtestEvent() {
    struct Telemetry cur_telem = Telemetry;
    const struct telem_layout *ptr = TELEMETRY_Type() == TELEM_DEVO ? devo8_layout : dsm_layout;
    for (int i = 0; ptr->source; ptr++, i++) {
        long cur_val = _TELEMETRY_GetValue(&cur_telem, ptr->source);
        long last_val = _TELEMETRY_GetValue(&tp.telem, ptr->source);
        struct LabelDesc *font;
        font = &TELEM_FONT;
        if (cur_val != last_val) {
            GUI_Redraw(&gui->value[i]);
        } else if(! TELEMETRY_IsUpdated(ptr->source)) {
            font = &TELEM_ERR_FONT;
        }
        GUI_SetLabelDesc(&gui->value[i], font);
        i++;
    }
    tp.telem = cur_telem;
}
