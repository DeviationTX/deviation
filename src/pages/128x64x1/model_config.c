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

#ifndef OVERRIDE_PLACEMENT
#include "common.h"
#include "pages.h"
#include "gui/gui.h"
#include "config/model.h"

enum {
    LABEL_X      = 0,
    LABEL_WIDTH  = 0,
    SELECT_X     = 63,
    SELECT_WIDTH = 60,
};
#endif //OVERRIDE_PLACEMENT

#include "../common/_model_config.c"

static struct modelcfg_obj * const gui = &gui_objs.u.modelcfg;

enum {
    ITEM_SWASHTYPE,
    ITEM_ELEINV,
    ITEM_AILINV,
    ITEM_COLINV,
    ITEM_ELEMIX,
    ITEM_AILMIX,
    ITEM_COLMIX,
    ITEM_LAST,
};

static void show_titlerow(const char *header)
{
    PAGE_ShowHeader(header);
    //u8 w = 40;
    // I don't think there is a need for the save button
    //GUI_CreateButtonPlateText(LCD_WIDTH - w -5, 0,
    //        w, ITEM_HEIGHT, &DEFAULT_FONT, NULL, 0x0000, okcancel_cb, _tr("Save"));
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    const void *label = NULL;
    void *value = NULL;
    void *tgl = NULL;
    switch(absrow) {
        case ITEM_SWASHTYPE:
            label = _tr("SwashType");
            value = swash_val_cb;
            break;
        case ITEM_ELEINV:
            label = _tr("ELE Inv");
            tgl = swashinv_press_cb; value = swashinv_val_cb; data = (void *)1L;
            break;
        case ITEM_AILINV:
            label = _tr("AIL Inv");
            tgl = swashinv_press_cb; value = swashinv_val_cb; data = (void *)2L;
            break;
        case ITEM_COLINV:
            label = _tr("COL Inv");
            tgl = swashinv_press_cb; value = swashinv_val_cb; data = (void *)4L;
            break;
        case ITEM_ELEMIX:
            label = _tr("ELE Mix");
            value = swashmix_val_cb; data = (void *)1L;
            break;
        case ITEM_AILMIX:
            label = _tr("AIL Mix");
            value = swashmix_val_cb; data = (void *)0L;
            break;
        case ITEM_COLMIX:
            label = _tr("COL Mix");
            value = swashmix_val_cb; data = (void *)2L;
            break;
    }
    GUI_CreateLabelBox(&gui->label[relrow], LABEL_X, y,
                LABEL_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, label);
    GUI_CreateTextSelectPlate(&gui->value[relrow], SELECT_X, y,
                SELECT_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, tgl, value, data);
    return 1;
}
void PAGE_ModelConfigInit(int page)
{
    (void)page;
    switch (Model.type) {
      case MODELTYPE_HELI: show_titlerow(_tr("Helicopter")); break; 
      case MODELTYPE_PLANE: show_titlerow(_tr("Airplane")); break; 
      case MODELTYPE_MULTI: show_titlerow(_tr("Multirotor")); break; 
      }

    if (Model.type == MODELTYPE_HELI) {
        GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, 
                         LCD_HEIGHT - HEADER_HEIGHT,
                         LINE_SPACE, ITEM_LAST, row_cb, NULL, NULL, NULL);
        GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, 0));
    }
}

static int row2_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    int idx = 0;
    int pos = 0;
    while(idx < absrow) {
        while(proto_strs[++pos])
            ;
        pos++;
        idx++;
    }
    GUI_CreateLabelBox(&gui->label[relrow], LABEL_X, y,
            LABEL_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr(proto_strs[pos]));
    GUI_CreateTextSelectPlate(&gui->value[relrow], SELECT_X, y,
            SELECT_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, NULL, proto_opt_cb, (void *)(long)absrow);
    return 1;
}

static int row3_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    void *ts;
    void *input_ts = NULL;
    void *ts_press = NULL;
    void *ts_data = NULL;
    char *label = NULL;
    void *label_cmd = NULL;

    switch (absrow) {
    case 0:
        label = _tr_noop("Center PW");
        ts = set_train_cb; ts_data = (void *)1L;
        break;
    case 1:
        label = _tr_noop("Delta PW");
        ts = set_train_cb; ts_data = (void *)2L;
        break;
    case 2:
        if (PPMin_Mode() != PPM_IN_SOURCE) {
            label = _tr_noop("Trainer Sw");
            ts = set_source_cb; ts_press = sourceselect_cb; ts_data = (void *)&Model.train_sw; input_ts = set_input_source_cb;
        } else {
            label = _tr_noop("Num Channels");
            ts = set_train_cb; ts_data = (void *)0L;
        }
        break;
    default:
        label_cmd = input_chname_cb; label = (void *)((long)absrow - 3);
        ts = set_chmap_cb; ts_data = label;
        break;
    }
    GUI_CreateLabelBox(&gui->label[relrow], LABEL_X, y,
            LABEL_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, label_cmd, NULL, label_cmd ? label : _tr(label));
    GUI_CreateTextSourcePlate(&gui->value[relrow], SELECT_X, y,
            SELECT_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, ts_press, ts, input_ts, ts_data);
    return 1;
}
#if HAS_VIDEO
static int row4_cb(int absrow, int relrow, int y, void *data)
{
    (void)data;
    void *ts = NULL;
    void *input_ts = NULL;
    void *ts_press = NULL;
    void *ts_data = NULL;
    char *label = NULL;

    switch (absrow) {
    case 0:
        label = _tr_noop("Video Enable");
        ts = set_source_cb; ts_press = sourceselect_cb; ts_data = (void *)&Model.videosrc; input_ts = set_input_source_cb;
        break;
    case 1:
        label = _tr_noop("Video Channel");
        ts = set_videoch_cb;
        break;
    case 2:
        label = _tr_noop("Contrast");
        ts = set_videocontrast_cb;
        break;
    case 3:
        label = _tr_noop("Brightness");
        ts = set_videobrightness_cb;
        break;
    }
    GUI_CreateLabelBox(&gui->label[relrow], LABEL_X, y,
            LABEL_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, NULL, NULL, _tr(label));
    GUI_CreateTextSourcePlate(&gui->value[relrow], SELECT_X, y,
            SELECT_WIDTH, LINE_HEIGHT, &DEFAULT_FONT, ts_press, ts, input_ts, ts_data);
    return 1;
}

void PAGE_VideoSetupInit(int page)
{
    (void)page;
    show_titlerow(_tr("Video"));
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                         LINE_SPACE, 4, row4_cb, NULL, NULL, NULL);
    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, 0));
}

#endif //HAS_VIDEO

void PAGE_ModelProtoInit(int page)
{
    (void) page;
    show_titlerow(ProtocolNames[Model.protocol]);

    proto_strs = PROTOCOL_GetOptions();
    int idx = 0;
    int pos = 0;
    while(idx < NUM_PROTO_OPTS) {
        if(proto_strs[pos] == NULL)
            break;
        while(proto_strs[++pos])
            ;
        pos++;
        idx++;
    }

    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                         LINE_SPACE, idx, row2_cb, NULL, NULL, NULL);
    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, 0));
}

void PAGE_TrainConfigInit(int page)
{
    (void)page;
    int mode = PPMin_Mode();
    show_titlerow(mode == PPM_IN_TRAIN1
                  ? _tr("Trainer Cfg (Channel)")
                  : mode == PPM_IN_TRAIN2
                    ? _tr("Trainer Cfg (Stick)")
                    : _tr("PPMIn Cfg (Extend)"));
    GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LCD_WIDTH, LCD_HEIGHT - HEADER_HEIGHT,
                         LINE_SPACE, PPMin_Mode() == PPM_IN_SOURCE ? 3 : 3 + MAX_PPM_IN_CHANNELS,
                         row3_cb, NULL, NULL, NULL);
    GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, 0));
}
