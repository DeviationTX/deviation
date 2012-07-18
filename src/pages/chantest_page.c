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

static guiObject_t *bar[NUM_CHANNELS];
static s16 showchan_cb(void *data);

void PAGE_ChantestInit(int page)
{
    (void)page;
    GUI_CreateLabel(8, 10, NULL, DEFAULT_FONT, "Channels");

    int i;
    for(i = 0; i < NUM_CHANNELS; i++) {
        bar[i] = GUI_CreateBarGraph(8 + 24 * i, 56, 10, 160, CHAN_MIN_VALUE, CHAN_MAX_VALUE, BAR_VERTICAL, showchan_cb, (void *)((long)i));
    }
}

void PAGE_ChantestEvent()
{
    int i;
    for(i = 0; i < NUM_CHANNELS; i++) {
        GUI_Redraw(bar[i]);
    }
}

int PAGE_ChantestCanChange()
{
    return 1;
}

static s16 showchan_cb(void *data)
{
    long ch = (long)data;
    return Channels[ch];
}

