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
#include "config/model.h"

#include "../common/_model_config.c"

static struct modelcfg_obj * const gui = &gui_objs.u.modelcfg;

static void okcancel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    PAGE_RemoveAllObjects();
    PAGE_ModelInit(-1);  // devo8 doesn't care the page value, while it must be -1 for devo10
}

static void show_titlerow(const char *header)
{
    GUI_CreateLabel(&gui->title, 8, 10, NULL, TITLE_FONT, (void *)header);
    PAGE_CreateOkButton(LCD_WIDTH-56, 4, okcancel_cb);
}

enum {
    COL1 = (8 + ((LCD_WIDTH - 320) / 2)),
    COL2 = (COL1 + 128),
    COL3 = (COL1 + 52),
    COL4 = (COL1 + 156),
    COL5 = (COL1 + 208),
    ROW1 = (40 + ((LCD_HEIGHT - 240) / 2)),
};

void MODELPAGE_Config()
{
    PAGE_SetModal(1);
    show_titlerow(Model.type == 0 ? _tr("Helicopter") : _tr("Airplane"));
    if (Model.type == 0) {
        u8 i = ROW1;
        GUI_CreateLabel(&gui->swashlbl, COL1, i, NULL, DEFAULT_FONT, _tr("SwashType"));
        GUI_CreateTextSelect(&gui->swash, COL2, i - 1, TEXTSELECT_96, NULL, swash_val_cb, NULL);
        i+=28;
        GUI_CreateLabel(&gui->invlbl[0], COL1, i, NULL, DEFAULT_FONT, _tr("ELE Inv"));
        GUI_CreateTextSelect(&gui->inv[0], COL2, i - 1, TEXTSELECT_96, swashinv_press_cb, swashinv_val_cb, (void *)1);
        i+=22;
        GUI_CreateLabel(&gui->invlbl[1], COL1, i, NULL, DEFAULT_FONT, _tr("AIL Inv"));
        GUI_CreateTextSelect(&gui->inv[1], COL2, i - 1, TEXTSELECT_96, swashinv_press_cb, swashinv_val_cb, (void *)2);
        i+=22;
        GUI_CreateLabel(&gui->invlbl[2], COL1, i, NULL, DEFAULT_FONT, _tr("COL Inv"));
        GUI_CreateTextSelect(&gui->inv[2], COL2, i - 1, TEXTSELECT_96, swashinv_press_cb, swashinv_val_cb, (void *)4);
        i+=28;
        GUI_CreateLabel(&gui->mixlbl[0], COL1, i, NULL, DEFAULT_FONT, _tr("ELE Mix"));
        GUI_CreateTextSelect(&gui->mix[0], COL2, i - 1, TEXTSELECT_96, NULL, swashmix_val_cb, (void *)1);
        i+=22;
        GUI_CreateLabel(&gui->mixlbl[1], COL1, i, NULL, DEFAULT_FONT, _tr("AIL Mix"));
        GUI_CreateTextSelect(&gui->mix[1], COL2, i - 1, TEXTSELECT_96, NULL, swashmix_val_cb, (void *)0);
        i+=22;
        GUI_CreateLabel(&gui->mixlbl[2], COL1, i, NULL, DEFAULT_FONT, _tr("COL Mix"));
        GUI_CreateTextSelect(&gui->mix[2], COL2, i - 1, TEXTSELECT_96, NULL, swashmix_val_cb, (void *)2);
    }
}

void MODELPROTO_Config()
{
    PAGE_SetModal(1);
    show_titlerow(ProtocolNames[Model.protocol]);
    proto_strs = PROTOCOL_GetOptions();
    int row = ROW1;
    int pos = 0;
    long idx = 0;
    while(idx < NUM_PROTO_OPTS) {
        if(proto_strs[pos] == NULL)
            break;
        GUI_CreateLabel(&gui->protolbl[idx], COL1, row, NULL, DEFAULT_FONT, _tr(proto_strs[pos]));
        GUI_CreateTextSelect(&gui->proto[idx], COL2, row, TEXTSELECT_96, NULL, proto_opt_cb, (void *)idx);
        while(proto_strs[++pos])
            ;
        pos++;
        idx++;
        row += 24;
    }
}

void MODELTRAIN_Config()
{
    PAGE_SetModal(1);
    int mode = PPMin_Mode();
    show_titlerow(mode == PPM_IN_TRAIN1
                  ? _tr("Trainer Cfg (Channel)")
                  : mode == PPM_IN_TRAIN2
                    ? _tr("Trainer Cfg (Stick)")
                    : _tr("PPMIn Cfg (Extend)"));
    int row = ROW1;
    if (PPMin_Mode() != PPM_IN_SOURCE) {
        GUI_CreateLabel(&gui->trainswlbl, COL1, row, NULL, DEFAULT_FONT, _tr("Trainer Sw"));
        GUI_CreateTextSelect(&gui->trainsw, COL2, row, TEXTSELECT_96, sourceselect_cb, set_source_cb, &Model.train_sw);
    } else {
        GUI_CreateLabel(&gui->numchlbl, COL1, row, NULL, DEFAULT_FONT, _tr("Num Channels"));
        GUI_CreateTextSelect(&gui->numch, COL2, row, TEXTSELECT_96, NULL, set_train_cb, (void *)0L);
    }
    row += 20;
    GUI_CreateLabel(&gui->centerpwlbl, COL1, row, NULL, DEFAULT_FONT, _tr("Center PW"));
    GUI_CreateTextSelect(&gui->centerpw, COL2, row, TEXTSELECT_96, NULL, set_train_cb, (void *)1L);
    row += 20;
    GUI_CreateLabel(&gui->deltapwlbl, COL1, row, NULL, DEFAULT_FONT, _tr("Delta PW"));
    GUI_CreateTextSelect(&gui->deltapw, COL2, row, TEXTSELECT_96, NULL, set_train_cb, (void *)2L);
    row += 20;
 
    if (PPMin_Mode() == PPM_IN_SOURCE)
        return;

    int num_rows= (MAX_PPM_IN_CHANNELS + 1) / 2;
    for (int i = 0; i < num_rows; i++) {
        long idx = i;
        row += 20;
        if (row > 300) // RBE: should be LCD_HEIGHT - 16(height box) ==> always < 300
            break;
        GUI_CreateLabelBox(&gui->ppmmaplbl[idx], COL1, row, 0, 16, &DEFAULT_FONT, input_chname_cb, NULL, (void *)idx);
        GUI_CreateTextSelect(&gui->ppmmap[idx], COL3, row, TEXTSELECT_96, NULL, set_chmap_cb, (void *)idx);
        idx += num_rows;
        if (idx >= MAX_PPM_IN_CHANNELS)
            break;
        GUI_CreateLabelBox(&gui->ppmmaplbl[idx], COL4, row, 0, 16, &DEFAULT_FONT, input_chname_cb, NULL, (void *)idx);
        GUI_CreateTextSelect(&gui->ppmmap[idx], COL5, row, TEXTSELECT_96, NULL, set_chmap_cb, (void *)idx);
    }
}

