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

#include "protocol/interface.h"
#include "rftools.h"

static struct scanner_page * const sp = &pagemem.u.scanner_page;

static void _draw_page(u8 enable);
static void _draw_channels(void);

static void _scan_enable(int enable)
{
    if (enable) {
        PROTOCOL_DeInit();
        sp->model_protocol = Model.protocol;  // Save protocol used in current Model file
        Model.protocol = PROTOCOL_SCANNER_CYRF;
        PROTOCOL_Init(1);  // Switch to scanner configuration and ignore safety
        PROTOCOL_SetBindState(0);  // Disable binding message
    } else {
        PROTOCOL_DeInit();
        Model.protocol = sp->model_protocol;
        PROTOCOL_Init(0);
    }
}

static void press_enable_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    sp->enable ^= 1;
    _scan_enable(sp->enable);
    GUI_Redraw(obj);
}

static const char *attenuator_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    Scanner.attenuator = GUI_TextSelectHelper(Scanner.attenuator, 0, 2, dir, 1, 1, NULL);
    switch (Scanner.attenuator) {
        case 0: return "0 dB";
        case 1: return "-20 dB";
        default: return "-50 dB";
    }
}

void PAGE_ScannerInit(int page)
{
    (void)page;
    memset(sp, 0, sizeof(struct scanner_page));
    PAGE_SetModal(0);
    _draw_page(1);
}

void PAGE_ScannerEvent()
{
    if(! sp->enable)
        return;

    // draw the channels
    _draw_channels();
}

void PAGE_ScannerExit()
{
    if (sp->enable)
        _scan_enable(0);
}
