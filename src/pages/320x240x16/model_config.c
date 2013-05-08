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

#define gui (&gui_objs.u.modelcfg)

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
    PAGE_CreateOkButton(264, 4, okcancel_cb);
}

void MODELPAGE_Config()
{
    PAGE_SetModal(1);
    show_titlerow(Model.type == 0 ? _tr("Helicopter") : _tr("Airplane"));
    if (Model.type == 0) {
        u8 i = 40;
        GUI_CreateLabel(&gui->swashlbl, 8, i, NULL, DEFAULT_FONT, _tr("SwashType"));
        GUI_CreateTextSelect(&gui->swash, 136, i, TEXTSELECT_96, NULL, swash_val_cb, NULL);
        i+=24;
        GUI_CreateLabel(&gui->invlbl[0], 8, i, NULL, DEFAULT_FONT, _tr("ELE Inv"));
        GUI_CreateTextSelect(&gui->inv[0], 136, i, TEXTSELECT_96, swashinv_press_cb, swashinv_val_cb, (void *)1);
        i+=24;
        GUI_CreateLabel(&gui->invlbl[1], 8, i, NULL, DEFAULT_FONT, _tr("AIL Inv"));
        GUI_CreateTextSelect(&gui->inv[1], 136, i, TEXTSELECT_96, swashinv_press_cb, swashinv_val_cb, (void *)2);
        i+=24;
        GUI_CreateLabel(&gui->invlbl[2], 8, i, NULL, DEFAULT_FONT, _tr("COL Inv"));
        GUI_CreateTextSelect(&gui->inv[2], 136, i, TEXTSELECT_96, swashinv_press_cb, swashinv_val_cb, (void *)4);
        i+=24;
        GUI_CreateLabel(&gui->mixlbl[0], 8, i, NULL, DEFAULT_FONT, _tr("ELE Mix"));
        GUI_CreateTextSelect(&gui->mix[0], 136, i, TEXTSELECT_96, NULL, swashmix_val_cb, (void *)1);
        i+=24;
        GUI_CreateLabel(&gui->mixlbl[1], 8, i, NULL, DEFAULT_FONT, _tr("AIL Mix"));
        GUI_CreateTextSelect(&gui->mix[1], 136, i, TEXTSELECT_96, NULL, swashmix_val_cb, (void *)0);
        i+=24;
        GUI_CreateLabel(&gui->mixlbl[2], 8, i, NULL, DEFAULT_FONT, _tr("COL Mix"));
        GUI_CreateTextSelect(&gui->mix[2], 136, i, TEXTSELECT_96, NULL, swashmix_val_cb, (void *)2);
    }
}

void MODELPROTO_Config()
{
    PAGE_SetModal(1);
    show_titlerow(ProtocolNames[Model.protocol]);
    proto_strs = PROTOCOL_GetOptions();
    int row = 40;
    int pos = 0;
    long idx = 0;
    while(idx < NUM_PROTO_OPTS) {
        if(proto_strs[pos] == NULL)
            break;
        GUI_CreateLabel(&gui->protolbl[idx], 8, row, NULL, DEFAULT_FONT, _tr(proto_strs[pos]));
        GUI_CreateTextSelect(&gui->proto[idx], 136, row, TEXTSELECT_96, NULL, proto_opt_cb, (void *)idx);
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
    show_titlerow((Model.num_ppmin & 0xC0) == 0x40
                  ? _tr("Trainer Config")
                  : _tr("PPMIn Config"));
    int row = 40;
    if (PPMin_Mode() == 1) {
        GUI_CreateLabel(&gui->trainswlbl, 8, row, NULL, DEFAULT_FONT, _tr("Trainer Sw"));
        GUI_CreateTextSelect(&gui->trainsw, 136, row, TEXTSELECT_96, sourceselect_cb, set_source_cb, &Model.train_sw);
    } else {
        GUI_CreateLabel(&gui->numchlbl, 8, row, NULL, DEFAULT_FONT, _tr("Num Channels"));
        GUI_CreateTextSelect(&gui->numch, 136, row, TEXTSELECT_96, NULL, set_train_cb, (void *)0L);
    }
    row += 20;
    GUI_CreateLabel(&gui->centerpwlbl, 8, row, NULL, DEFAULT_FONT, _tr("Center PW"));
    GUI_CreateTextSelect(&gui->centerpw, 136, row, TEXTSELECT_96, NULL, set_train_cb, (void *)1L);
    row += 20;
    GUI_CreateLabel(&gui->deltapwlbl, 8, row, NULL, DEFAULT_FONT, _tr("Delta PW"));
    GUI_CreateTextSelect(&gui->deltapw, 136, row, TEXTSELECT_96, NULL, set_train_cb, (void *)2L);
    row += 20;
 
    if ((Model.num_ppmin & 0xC0) != 0x40)
        return;

    int num_rows= (MAX_PPM_IN_CHANNELS + 1) / 2;
    for (int i = 0; i < num_rows; i++) {
        long idx = i;
        row += 20;
        if (row > 300)
            break;
        GUI_CreateLabelBox(&gui->ppmmaplbl[idx], 8, row, 0, 16, &DEFAULT_FONT, input_chname_cb, NULL, (void *)idx);
        GUI_CreateTextSelect(&gui->ppmmap[idx], 60, row, TEXTSELECT_96, NULL, set_chmap_cb, (void *)idx);
        idx += num_rows;
        if (idx >= MAX_PPM_IN_CHANNELS)
            break;
        GUI_CreateLabelBox(&gui->ppmmaplbl[idx], 164, row, 0, 16, &DEFAULT_FONT, input_chname_cb, NULL, (void *)idx);
        GUI_CreateTextSelect(&gui->ppmmap[idx], 216, row, TEXTSELECT_96, NULL, set_chmap_cb, (void *)idx);
    }
}

