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
#include "icons.h"
#include "gui/gui.h"
#include "config/model.h"

#define tp pagemem.u.toggle_select_page
#define gui5 (&gui_objs.u.maincfg.u.g5)

#include "../common/_toggle_select.c"

static void show_iconsel_page(int idx);
extern const struct LabelDesc outline;

void tglico_select_cb(guiObject_t *obj, s8 press_type, const void *data)
{
    (void)obj;
    if (press_type == -1) {
        // --> data = (ToggleNumber << 12) | (IconNumber << 8) | IconPosition
        u8 IconPosition = ((long)data      ) & 0xff;
        u8 IconNumber   = ((long)data >> 8 ) & 0x0f;
        Model.pagecfg.tglico[tp.tglidx][IconNumber] = IconPosition + 1;
        show_iconsel_page(IconNumber);
    }
}

static void tglico_cancel_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    memcpy(Model.pagecfg.tglico[tp.tglidx], tp.tglicons, sizeof(tp.tglicons));
    PAGE_RemoveAllObjects();
    PAGE_MainCfgInit(1);
}

static void tglico_ok_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    PAGE_RemoveAllObjects();
    PAGE_MainCfgInit(1);
}

static void show_iconsel_page(int SelectedIcon)
{
    struct ImageMap img;
    u16 w, h, x, y;
    u8 i, j;
    PAGE_RemoveAllObjects();
    PAGE_SetModal(1);
    LCD_ImageDimensions(TOGGLE_FILE, &w, &h);
    u8 rows = h / TOGGLEICON_HEIGHT;        // icons in rows
    u8 count = w / TOGGLEICON_WIDTH * rows;        // number of icons
    PAGE_CreateCancelButton(216, 4, tglico_cancel_cb);
    // Ok-Button for saving
    PAGE_CreateOkButton(165, 4, tglico_ok_cb);
    // Show name of source for toggle icon
    u8 toggleinput = MIXER_SRC(Model.pagecfg.toggle[tp.tglidx]);

    // style the switch textbox
    struct LabelDesc outline = { DEFAULT_FONT.font, 0, 0, DEFAULT_FONT.font_color, LABEL_TRANSPARENT };
    GUI_CreateRect(&gui5->toggleframe, 80+80*SelectedIcon, 39, 77, 33, &outline);
    long posX, posY;

    GUI_CreateLabelBox(&gui5->switchbox,  4, 47, 70, 22, &NORMALBOX_FONT, NULL, NULL,
                       INPUT_SourceNameAbbrevSwitch(tp.str, Model.pagecfg.toggle[tp.tglidx]));

    int num_positions = INPUT_NumSwitchPos(toggleinput);
    if(num_positions < 2)
        num_positions = 2;

    GUI_CreateLabel(&gui5->togglelabel[0], 94, 50, NULL, DEFAULT_FONT, _tr("Pos 0:"));
    img = TGLICO_GetImage(Model.pagecfg.tglico[tp.tglidx][0]);
    GUI_CreateImageOffset(&gui5->toggleicon[0], 124, 40, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT, img.x_off, img.y_off, img.file,
             SelectedIcon == 0 ? tglico_reset_cb : tglico_setpos_cb, (void *)0L);

    GUI_CreateLabel(&gui5->togglelabel[1], 174, 50, NULL, DEFAULT_FONT, _tr("Pos 1:"));
    img = TGLICO_GetImage(Model.pagecfg.tglico[tp.tglidx][1]);
    GUI_CreateImageOffset(&gui5->toggleicon[1], 204, 40, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT, img.x_off, img.y_off, img.file,
             SelectedIcon == 1 ? tglico_reset_cb : tglico_setpos_cb, (void *)1L);
    if (num_positions == 3) {
        GUI_CreateLabel(&gui5->togglelabel[2], 254, 50, NULL, DEFAULT_FONT, _tr("Pos 2:"));
        img = TGLICO_GetImage(Model.pagecfg.tglico[tp.tglidx][2]);
        GUI_CreateImageOffset(&gui5->toggleicon[2], 284, 40, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT, img.x_off, img.y_off, img.file,
             SelectedIcon == 2 ? tglico_reset_cb : tglico_setpos_cb, (void *)2L);
    }
    u8 cursel = Model.pagecfg.tglico[tp.tglidx][SelectedIcon] - 1;
    long pos = 0;
    posX = 0;
    posY = 0;
    for(j = 0; j < 4; j++) {
        y = 80 + j * 40;
        for(i = 0; i < 8; i++,pos++,posX+=TOGGLEICON_WIDTH) {
            if (pos >= count) break;
            x = 4 + i*(TOGGLEICON_WIDTH+8);
            if (pos == cursel) GUI_CreateRect(&gui5->symbolframe, x-1, y-1, TOGGLEICON_WIDTH+2, TOGGLEICON_HEIGHT+2, &outline);
            while (posX >= w) {
                posX -= w;
                posY += TOGGLEICON_HEIGHT;
            }
            GUI_CreateImageOffset(&gui5->symbolicon[pos], x, y, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT, posX, posY, TOGGLE_FILE,
                                    tglico_select_cb, (void *)((long)(SelectedIcon << 8) | pos));
        }
    }
}
