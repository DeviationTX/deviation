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
#include "config/model.h"

#include "../common/_mixer_setup.c"

#define COL1_TEXT   4
#define COL1_VALUE  56
#define COL2_TEXT  164
#define COL2_VALUE 216

static void _show_titlerow()
{
    GUI_CreateLabel(4, 10, MIXPAGE_ChanNameProtoCB, TITLE_FONT, (void *)((long)mp->cur_mixer->dest));
    GUI_CreateTextSelect(COL1_VALUE, 8, TEXTSELECT_96, 0x0000, NULL, templatetype_cb, (void *)((long)mp->channel));
    PAGE_CreateCancelButton(160, 4, okcancel_cb);
    PAGE_CreateOkButton(264, 4, okcancel_cb);
}

static void _show_simple()
{
    //Row 1
    mp->firstObj = GUI_CreateLabel(COL1_TEXT, 40, NULL, DEFAULT_FONT, _tr("Src:"));
    GUI_CreateTextSelect(COL1_VALUE, 40, TEXTSELECT_96, 0x0000, sourceselect_cb, set_source_cb, &mp->mixer[0].src);
    GUI_CreateLabel(COL2_TEXT, 40, NULL, DEFAULT_FONT, _tr("Curve:"));
    GUI_CreateTextSelect(COL2_VALUE, 40, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, &mp->mixer[0]);
    //Row 2
    mp->graphs[0] = GUI_CreateXYGraph(112, 64, 120, 120,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, touch_cb, &mp->mixer[0]);
    //Row 4
    GUI_CreateLabel(COL1_TEXT, 192, NULL, DEFAULT_FONT, _tr("Scale:"));
    GUI_CreateTextSelect(COL1_VALUE, 192, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->mixer[0].scalar);
    GUI_CreateLabel(COL2_TEXT, 192, NULL, DEFAULT_FONT, _tr("Offset:"));
    GUI_CreateTextSelect(COL2_VALUE, 192, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->mixer[0].offset);
    //Row 5
    /*
    mp->trimObj = GUI_CreateButton(COL1_VALUE, 214, BUTTON_96x16, show_trim_cb, 0x0000, toggle_trim_cb, NULL);
    if (! MIXER_SourceHasTrim(MIXER_SRC(mp->mixer[0].src)))
        GUI_SetHidden(mp->trimObj, 1);

    GUI_CreateLabel(COL1_TEXT, 216, NULL, DEFAULT_FONT, _tr("Min:"));
    GUI_CreateTextSelect(COL1_VALUE, 216, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->limit.min);
    GUI_CreateLabel(COL2_TEXT, 216, NULL, DEFAULT_FONT, _tr("Max:"));
    GUI_CreateTextSelect(COL2_VALUE, 216, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->limit.max);
    */
}

static void _show_expo_dr()
{
    sync_mixers();
    //Row 1
    mp->firstObj = GUI_CreateLabelBox(COL1_TEXT, 32, 96, 16, &NARROW_FONT, NULL, NULL, _tr("Src"));
    /*
    mp->trimObj = GUI_CreateButton(COL1_TEXT, 32, BUTTON_96x16, show_trim_cb, 0x0000, toggle_trim_cb, NULL);
    mp->firstObj = mp->trimObj;

    if (! MIXER_SourceHasTrim(MIXER_SRC(mp->mixer[0].src)))
        GUI_SetHidden(mp->trimObj, 1);
    */

    GUI_CreateLabelBox(112, 32, 96, 16, &NARROW_FONT, NULL, NULL, _tr("Switch1"));
    GUI_CreateLabelBox(216, 32, 96, 16, &NARROW_FONT, NULL, NULL, _tr("Switch2"));
    //Row 2
    GUI_CreateTextSelect(COL1_TEXT, 48, TEXTSELECT_96, 0x0000, sourceselect_cb, set_source_cb, &mp->mixer[0].src);
    GUI_CreateTextSelect(112, 48, TEXTSELECT_96, 0x0000, sourceselect_cb, set_drsource_cb, &mp->mixer[1].sw);
    GUI_CreateTextSelect(216, 48, TEXTSELECT_96, 0x0000, sourceselect_cb, set_drsource_cb, &mp->mixer[2].sw);
    //Row 3
    GUI_CreateLabelBox(COL1_TEXT, 72, 96, 16, &NARROW_FONT, NULL, NULL, _tr("High-Rate"));
    mp->expoObj[0] = GUI_CreateButton(112, 72, BUTTON_96x16, show_rate_cb, 0x0000, toggle_link_cb, (void *)0);
    mp->expoObj[4] = GUI_CreateButton(216, 72, BUTTON_96x16, show_rate_cb, 0x0000, toggle_link_cb, (void *)1);
    //Row 4
    GUI_CreateTextSelect(COL1_TEXT, 96, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, &mp->mixer[0]);
    //The following 2 items are mutex.  One is always hidden
    mp->expoObj[1] = GUI_CreateLabelBox(112, 96, 96, 16, &NARROW_FONT, NULL, NULL, _tr("Linked"));
    mp->expoObj[2] = GUI_CreateTextSelect(112, 96, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, &mp->mixer[1]);
    //The following 2 items are mutex.  One is always hidden
    mp->expoObj[5] = GUI_CreateLabelBox(216, 96, 96, 16, &NARROW_FONT, NULL, NULL, _tr("Linked"));
    mp->expoObj[6] = GUI_CreateTextSelect(216, 96, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, &mp->mixer[2]);
    //Row 5
    GUI_CreateLabel(COL1_TEXT, 122, NULL, DEFAULT_FONT, _tr("Scale:"));
    GUI_CreateTextSelect(40, 120, TEXTSELECT_64, 0x0000, NULL, set_number100_cb, &mp->mixer[0].scalar);
    mp->expoObj[3] = GUI_CreateTextSelect(112, 120, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->mixer[1].scalar);
    mp->expoObj[7] = GUI_CreateTextSelect(216, 120, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->mixer[2].scalar);

    mp->graphs[0] = GUI_CreateXYGraph(COL1_TEXT, 140, 96, 96,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, touch_cb, &mp->mixer[0]);
    mp->graphs[1] = GUI_CreateXYGraph(112, 140, 96, 96,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, touch_cb, &mp->mixer[1]);
    mp->graphs[2] = GUI_CreateXYGraph(216, 140, 96, 96,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, touch_cb, &mp->mixer[2]);

    //Enable/Disable the relevant widgets
    update_rate_widgets(0);
    update_rate_widgets(1);
}

static void _show_complex()
{
    //Row 1
    if (! mp->expoObj[0]) {
        mp->firstObj = GUI_CreateLabel(COL1_TEXT, 40, NULL, DEFAULT_FONT, _tr("Mixers:"));
        GUI_CreateTextSelect(COL1_VALUE, 40, TEXTSELECT_96, 0x0000, NULL, set_nummixers_cb, NULL);
        GUI_CreateLabel(COL2_TEXT, 40, NULL, DEFAULT_FONT, _tr("Page:"));
        GUI_CreateTextSelect(COL2_VALUE, 40, TEXTSELECT_96, 0x0000, reorder_cb, set_mixernum_cb, NULL);
    } else {
        GUI_RemoveHierObjects(mp->expoObj[0]);
    }
    //Row 2
    mp->expoObj[0] = GUI_CreateLabel(COL1_TEXT, 64, NULL, DEFAULT_FONT, _tr("Switch:"));
    GUI_CreateTextSelect(COL1_VALUE, 64, TEXTSELECT_96, 0x0000, sourceselect_cb, set_source_cb, &mp->cur_mixer->sw);
    GUI_CreateLabel(COL2_TEXT, 64, NULL, DEFAULT_FONT, _tr("Mux:"));
    GUI_CreateTextSelect(COL2_VALUE, 64, TEXTSELECT_96, 0x0000, NULL, set_mux_cb, NULL);
    //Row 3
    GUI_CreateLabel(COL1_TEXT, 98, NULL, DEFAULT_FONT, _tr("Src:"));
    GUI_CreateTextSelect(COL1_VALUE, 98, TEXTSELECT_96, 0x0000, sourceselect_cb, set_source_cb, &mp->cur_mixer->src);
    //Row 4
    GUI_CreateLabel(COL1_TEXT, 122, NULL, DEFAULT_FONT, _tr("Curve:"));
    GUI_CreateTextSelect(COL1_VALUE, 122, TEXTSELECT_96, 0x0000, curveselect_cb, set_curvename_cb, mp->cur_mixer);
    //Row 5
    GUI_CreateLabel(COL1_TEXT, 156, NULL, DEFAULT_FONT, _tr("Scale:"));
    GUI_CreateTextSelect(COL1_VALUE, 156, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->cur_mixer->scalar);
    //Row 6
    GUI_CreateLabel(COL1_TEXT, 180, NULL, DEFAULT_FONT, _tr("Offset:"));
    GUI_CreateTextSelect(COL1_VALUE, 180, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->cur_mixer->offset);
    mp->graphs[1] = GUI_CreateBarGraph(COL2_TEXT, 88, 10, 120,
                              CHAN_MIN_VALUE, CHAN_MAX_VALUE, BAR_VERTICAL,
                              eval_chan_cb, NULL);
    mp->graphs[0] = GUI_CreateXYGraph(192, 88, 120, 120,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                              0, 0, eval_mixer_cb, curpos_cb, touch_cb, mp->cur_mixer);
    //Row 7
    GUI_CreateButton(COL1_VALUE, 214, BUTTON_96x16, show_trim_cb, 0x0000, toggle_trim_cb, NULL);
    mp->trimObj = GUI_CreateButton(COL1_VALUE, 214, BUTTON_96x16, show_trim_cb, 0x0000, toggle_trim_cb, NULL);
    if (! MIXER_SourceHasTrim(MIXER_SRC(mp->mixer[0].src)))
        GUI_SetHidden(mp->trimObj, 1);
    /*
    GUI_CreateLabel(COL1_TEXT, 216, NULL, DEFAULT_FONT, _tr("Min:"));
    GUI_CreateTextSelect(COL1_VALUE, 216, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->limit.min);
    GUI_CreateLabel(COL2_TEXT, 216, NULL, DEFAULT_FONT, _tr("Max:"));
    GUI_CreateTextSelect(COL2_VALUE, 216, TEXTSELECT_96, 0x0000, NULL, set_number100_cb, &mp->limit.max);
    */
}
