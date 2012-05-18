/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Foobar is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "target.h"
#include "gui/gui.h"

static const char *templates[] = {
    "None",
    "Simple",
    "Dual Rate",
    "Complex"
};

static const char *channel_name[] = {
    "Ch1", "Ch2", "Ch3", "Ch4",
    "Ch5", "Ch6", "Ch7", "Ch8",
    "Ch9", "Ch10", "Ch11" "Ch12",
    "Ch13", "Ch14", "Ch15", "Ch16"
};

#define ENTRIES_PER_PAGE 8

static char input_str[ENTRIES_PER_PAGE][6];
static char switch_str[ENTRIES_PER_PAGE][7];
static u8 cur_template[ENTRIES_PER_PAGE];

static const char *mixertype_cb(guiObject_t *obj, int value, void *data);

static const char *inp[] = {
    "THR", "RUD", "ELE", "AIL",
    "", "D/R", "D/R-C1", "D/R-C2"
};

void PAGE_MixerInit(int page)
{
    int init_y = 16;
    int i;
    for (i = 0; i < ENTRIES_PER_PAGE; i++) {
        void *ptr = (void *)((long)i);
        int row = init_y + 26 * i;
        cur_template[i] = 0;
        GUI_CreateLabel(10, row, channel_name[ENTRIES_PER_PAGE * page + i], 0x0000);
        strcpy(input_str[i], (i < 4) ? inp[i] : "");
        GUI_CreateLabel(40, row, input_str[i], 0x0000);
        GUI_CreateTextSelect(100, row, 128, 16, 0x0000, NULL, mixertype_cb, ptr);
        strcpy(switch_str[i], (i < 4) ? inp[i+4] : "");
        GUI_CreateLabel(240, row, switch_str[i], 0x0000);
    }
    GUI_DrawScreen();
}

void PAGE_MixerEvent()
{
}

static const char *mixertype_cb(guiObject_t *obj, int value, void *data)
{
    long idx = (long)data;
    if(value > 0) {
        if(cur_template[idx] < 3)
            cur_template[idx]++;
    } else if(value < 0) {
        if(cur_template[idx])
            cur_template[idx]--;
    }
    return templates[cur_template[idx]];
}
