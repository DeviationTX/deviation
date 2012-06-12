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

#include "target.h"
#include "pages.h"
#include "gui/gui.h"

static struct main_page * const mp = &pagemem.u.main_page;
s16 trim_cb(void * data);
const char *show_throttle_cb(guiObject_t *obj, void *data);
s16 trim_cb(void * data);
extern s16 Channels[NUM_CHANNELS];
extern s8 Trims[NUM_TRIMS];

void PAGE_MainInit(int page)
{
    (void)page;
    int i;
    for (i = 0; i < TRIMS_TO_SHOW; i++)
        mp->trims[i] = Trims[i];
    mp->throttle = Channels[0];
    mp->trimObj[0] = GUI_CreateBarGraph(10, 50, 10, 140, -100, 100, TRIM_VERTICAL, trim_cb, &Trims[0]);
    mp->trimObj[1] = GUI_CreateBarGraph(30, 220, 125, 10, -100, 100, TRIM_HORIZONTAL, trim_cb, &Trims[1]);
    mp->trimObj[2] = GUI_CreateBarGraph(300, 50, 10, 140, -100, 100, TRIM_VERTICAL, trim_cb, &Trims[2]);
    mp->trimObj[3] = GUI_CreateBarGraph(165, 220, 125, 10, -100, 100, TRIM_HORIZONTAL, trim_cb, &Trims[3]);
    LCD_SetFont(9);
    mp->throttleObj = GUI_CreateLabel(50, 120, show_throttle_cb, 0x0000, &Channels[0]);
    LCD_SetFont(6);
    GUI_DrawScreen();
}

void PAGE_MainEvent()
{
    int i;
    for(i = 0; i < TRIMS_TO_SHOW; i++) {
        if (mp->trims[i] != Trims[i]) {
            mp->trims[i] = Trims[i];
            GUI_Redraw(mp->trimObj[i]);
        }
    }
    
    if(mp->throttle != Channels[0]) {
        mp->throttle = Channels[0];
        GUI_Redraw(mp->throttleObj);
    }
}


int PAGE_MainCanChange()
{
    return 1;
}

const char *show_throttle_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    s16 *throttle = (s16 *)data;
    sprintf(mp->tmpstr, "%3d%%", RANGE_TO_PCT(*throttle));
    return mp->tmpstr;
}

s16 trim_cb(void * data)
{
    s8 *trim = (s8 *)data;
    return *trim;
}
