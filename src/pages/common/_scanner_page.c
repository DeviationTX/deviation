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

static struct scanner_page * const sp = &pagemem.u.scanner_page;

static void _draw_page(u8 enable);
static void _draw_channels(void);

#ifdef ENABLE_MODULAR
#error "Not supported in MODULAR build"
#endif

// The high level interface to do the scan
static void _scan_enable(int enable)
{
    if (enable) {
        PROTOCOL_DeInit();
        SCANNER_CYRF_Cmds(0);  // Switch to SCANNER_CYRF configuration
        PROTOCOL_SetBindState(0);  // Disable binding message
    } else {
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
    (void)data;
    GUI_TextSelectEnablePress((guiTextSelect_t *)obj, sp->attenuator);
    sp->attenuator = GUI_TextSelectHelper(sp->attenuator, 0, 2, dir, 1, 1, NULL);
    switch (sp->attenuator) {
        case 0: return "0 dB";
        case 1: return "-20dB";
        default: return "-50 dB";
    }
}
static const char *average_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    GUI_TextSelectEnablePress((guiTextSelect_t *)obj, sp->averaging);
    sp->averaging = GUI_TextSelectHelper(sp->averaging, 0, 255, dir, 1, 10, NULL);
    if (sp->averaging == 0)
        strcpy(tempstring, _tr("Peak"));
    else
        snprintf(tempstring, 8, "Avg %d", sp->averaging);
    return tempstring;
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
