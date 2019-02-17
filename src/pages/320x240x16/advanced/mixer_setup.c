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
#include "../pages.h"
#include "config/model.h"

#include "../../common/advanced/_mixer_setup.c"

static struct advmixcfg_obj_g1 * const gui1 = &gui_objs.u.advmixcfg.u.g1;
static struct advmixcfg_obj_g2 * const gui2 = &gui_objs.u.advmixcfg.u.g2;
static struct advmixcfg_obj_g3 * const gui3 = &gui_objs.u.advmixcfg.u.g3;

#if LCD_WIDTH == 320
    //320x240
enum {
     COL1        =  10,
     COL2        =  56,
     COL3        = ((310 - 120 - (COL2 + 106)) / 2 + COL2 + 106),
     COL_SCALEHI =  36,
     COL_EXP2    = 112,
     COL_EXP3    = 216,
     COL_GRAPH   = 192,
     COL1_TEXT   =   4,
     COL1_VALUE  =  56,
     COL2_TEXT   = 164,
     COL2_VALUE  = 216,
     EXP_WIDTH   =  77,
     EXP_HEIGHT  =  96,
};
#else
    // 480x227
enum {
     COL1        = 90,
     COL2        = 136,
     COL3        = ((310 - 120 - (COL2 + 106)) / 2 + COL2 + 106 + 80),
     COL_SCALEHI = 116,
     COL_EXP2    = 192,
     COL_EXP3    = 296,
     COL_GRAPH   = 272,
     COL1_TEXT   =  84,
     COL1_VALUE  = 136,
     COL2_TEXT   = 244,
     COL2_VALUE  = 296,
     EXP_WIDTH   =  96,
     EXP_HEIGHT  = 120,
};
#endif
static const int COL_TEMPLATE = 56;
static const int GRAPH_Y = ((220 - 40 - 150) / 2 + 40);

static void _show_titlerow()
{
    GUI_CreateLabelBox(&gui->chan, 4, 10, COL_TEMPLATE - 4, 14, &TITLE_FONT, MIXPAGE_ChanNameProtoCB, NULL, (void *)((long)mp->cur_mixer->dest));
    GUI_CreateTextSelect(&gui->tmpl, COL_TEMPLATE, 8, TEXTSELECT_96, NULL, templatetype_cb, (void *)((long)mp->channel));
    PAGE_CreateCancelButton(LCD_WIDTH-160, 4, okcancel_cb);
    PAGE_CreateOkButton(LCD_WIDTH-56, 4, okcancel_cb);
}

static void _show_simple()
{
    const int space = 40;
    int y = 60;
    //Row 1
    mp->firstObj = GUI_CreateLabelBox(&gui1->srclbl, COL1_TEXT, y, COL1_VALUE - COL1_TEXT, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Src"));
    GUI_CreateTextSource(&gui1->src, COL1_VALUE, y, TEXTSELECT_96, sourceselect_cb, set_source_cb, set_input_source_cb, &mp->mixer[0].src);
    y += space;
    //Row 2
    GUI_CreateLabelBox(&gui1->curvelbl, COL1_TEXT, y, COL1_VALUE - COL1_TEXT, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Curve"));
    GUI_CreateTextSelect(&gui1->curve, COL1_VALUE, y, TEXTSELECT_96, curveselect_cb, set_curvename_cb, &mp->mixer[0]);
    y += space;

    GUI_CreateXYGraph(&gui1->graph, COL3, GRAPH_Y, 120, 150,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE * 125 / 100,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE * 125 / 100,
                              0, PCT_TO_RANGE(25), eval_mixer_cb, curpos_cb, touch_cb, &mp->mixer[0]);
    //Row 4
    GUI_CreateLabelBox(&gui1->scalelbl, COL1_TEXT, y, COL1_VALUE - COL1_TEXT, 0, &LABEL_FONT, scalestring_cb, NULL, (void *)0);
    GUI_CreateTextSelect(&gui1->scale, COL1_VALUE, y, TEXTSELECT_96, NULL, set_number100_cb, &mp->mixer[0].scalar);
    y += space;
    //Row 4
    GUI_CreateLabelBox(&gui1->offsetlbl, COL1_TEXT, y, COL1_VALUE - COL1_TEXT, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Offset"));
    GUI_CreateTextSelect(&gui1->offset, COL1_VALUE, y, TEXTSELECT_96, NULL, set_number100_cb, &mp->mixer[0].offset);
}

static void _show_expo_dr()
{
    sync_mixers();
    //Row 1
    mp->firstObj = GUI_CreateLabelBox(&gui2->srclbl, COL1_TEXT, 32, 96, 16, &NARROW_FONT, GUI_Localize, NULL, _tr_noop("Src"));
    GUI_CreateLabelBox(&gui2->sw1lbl, COL_EXP2, 32, 96, 16, &NARROW_FONT, GUI_Localize, NULL, _tr_noop("Switch1"));
    GUI_CreateLabelBox(&gui2->sw2lbl, COL_EXP3, 32, 96, 16, &NARROW_FONT, GUI_Localize, NULL, _tr_noop("Switch2"));
    //Row 2
    GUI_CreateTextSource(&gui2->src, COL1_TEXT, 48, TEXTSELECT_96, sourceselect_cb, set_source_cb,
                         set_input_source_cb, &mp->mixer[0].src);
    GUI_CreateTextSource(&gui2->sw1, COL_EXP2, 48, TEXTSELECT_96, sourceselect_cb, set_drsource_cb,
                         set_input_source_cb, &mp->mixer[1].sw);
    GUI_CreateTextSource(&gui2->sw2, COL_EXP3, 48, TEXTSELECT_96, sourceselect_cb, set_drsource_cb,
                         set_input_source_cb, &mp->mixer[2].sw);
    //Row 3
    GUI_CreateLabelBox(&gui2->high, COL1_TEXT, 72, 96, 16, &NARROW_FONT, GUI_Localize, NULL, _tr_noop("High-Rate"));
    GUI_CreateButton(&gui2->rate[0], COL_EXP2, 72, BUTTON_96x16, show_rate_cb, toggle_link_cb, (void *)0);
    GUI_CreateButton(&gui2->rate[1], COL_EXP3, 72, BUTTON_96x16, show_rate_cb, toggle_link_cb, (void *)1);
    //Row 4
    GUI_CreateTextSelect(&gui2->curvehi, COL1_TEXT, 96, TEXTSELECT_96, curveselect_cb, set_curvename_cb, &mp->mixer[0]);
    //The following 2 items are mutex.  One is always hidden
    GUI_CreateLabelBox(&gui2->linked[0], COL_EXP2, 96, 96, 16, &NARROW_FONT, GUI_Localize, NULL, _tr_noop("Linked"));
    GUI_CreateTextSelect(&gui2->curve[0], COL_EXP2, 96, TEXTSELECT_96, curveselect_cb, set_curvename_cb, &mp->mixer[1]);
    //The following 2 items are mutex.  One is always hidden
    GUI_CreateLabelBox(&gui2->linked[1], COL_EXP3, 96, 96, 16, &NARROW_FONT, GUI_Localize, NULL, _tr_noop("Linked"));
    GUI_CreateTextSelect(&gui2->curve[1], COL_EXP3, 96, TEXTSELECT_96, curveselect_cb, set_curvename_cb, &mp->mixer[2]);
    //Row 5
    GUI_CreateLabelBox(&gui2->scalelbl, COL1_TEXT, 122, COL_SCALEHI - COL1_TEXT, 0, &LABEL_FONT, scalestring_cb, NULL, (void *)0);
    GUI_CreateTextSelect(&gui2->scalehi, COL_SCALEHI, 120, TEXTSELECT_64, NULL, set_number100_cb, &mp->mixer[0].scalar);
    GUI_CreateTextSelect(&gui2->scale[0], COL_EXP2, 120, TEXTSELECT_96, NULL, set_number100_cb, &mp->mixer[1].scalar);
    GUI_CreateTextSelect(&gui2->scale[1], COL_EXP3, 120, TEXTSELECT_96, NULL, set_number100_cb, &mp->mixer[2].scalar);

    GUI_CreateXYGraph(&gui2->graphhi, COL1_TEXT + (LCD_WIDTH == 320 ? 10 : 0), 140, EXP_WIDTH, EXP_HEIGHT,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE * 5 / 4,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE * 5 / 4,
                              0, PCT_TO_RANGE(25), eval_mixer_cb, curpos_cb, touch_cb, &mp->mixer[0]);
    GUI_CreateXYGraph(&gui2->graph[0], COL_EXP2 + (LCD_WIDTH == 320 ? 10 : 0), 140, EXP_WIDTH, EXP_HEIGHT,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE * 5 / 4,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE * 5 / 4,
                              0, PCT_TO_RANGE(25), eval_mixer_cb, curpos_cb, touch_cb, &mp->mixer[1]);
    GUI_CreateXYGraph(&gui2->graph[1], COL_EXP3 + (LCD_WIDTH == 320 ? 10 : 0), 140, EXP_WIDTH, EXP_HEIGHT,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE * 5/ 4,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE * 5 /4,
                              0, PCT_TO_RANGE(25), eval_mixer_cb, curpos_cb, touch_cb, &mp->mixer[2]);

    //Enable/Disable the relevant widgets
    _update_rate_widgets(0);
    _update_rate_widgets(1);
}

static void _show_complex(int page_change)
{
    (void)page_change;
    //Row 1
    if (! mp->firstObj) {
        mp->firstObj = GUI_CreateLabelBox(&gui3->nummixlbl, COL1_TEXT, 40, COL1_VALUE - COL1_TEXT, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Mixers"));
        GUI_CreateTextSelect(&gui3->nummix, COL1_VALUE, 40, TEXTSELECT_96, NULL, set_nummixers_cb, NULL);
        GUI_CreateLabelBox(&gui3->pagelbl, COL2_TEXT, 40, COL2_VALUE - COL2_TEXT, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Page"));
        guiObject_t *obj = GUI_CreateTextSelect(&gui3->page, COL2_VALUE, 40, TEXTSELECT_96, reorder_cb, set_mixernum_cb, NULL);
        if (! GUI_GetSelected()) //Set the page button to be selected if nothinge else is yet
            GUI_SetSelected(obj);
    } else {
        GUI_RemoveHierObjects((guiObject_t *)&gui3->swlbl);
    }
    //Row 2
    GUI_CreateLabelBox(&gui3->swlbl, COL1_TEXT, 64, COL1_VALUE - COL1_TEXT, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Switch"));
    GUI_CreateTextSource(&gui3->sw, COL1_VALUE, 64, TEXTSELECT_96, sourceselect_cb, set_drsource_cb,
                         set_input_source_cb, &mp->cur_mixer->sw);
    GUI_CreateLabelBox(&gui3->muxlbl, COL2_TEXT, 64, COL2_VALUE - COL2_TEXT, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Mux"));
    GUI_CreateTextSelect(&gui3->mux, COL2_VALUE, 64, TEXTSELECT_96, NULL, set_mux_cb, NULL);
    //Row 3
    GUI_CreateLabelBox(&gui3->srclbl, COL1_TEXT, 98, COL1_VALUE - COL1_TEXT, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Src"));
    GUI_CreateTextSource(&gui3->src, COL1_VALUE, 98, TEXTSELECT_96, sourceselect_cb, set_source_cb,
                         set_input_source_cb, &mp->cur_mixer->src);
    //Row 4
    GUI_CreateLabelBox(&gui3->curvelbl, COL1_TEXT, 122, COL1_VALUE - COL1_TEXT, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Curve"));
    GUI_CreateTextSelect(&gui3->curve, COL1_VALUE, 122, TEXTSELECT_96, curveselect_cb, set_curvename_cb, mp->cur_mixer);
    //Row 5
    GUI_CreateLabelBox(&gui3->scalelbl, COL1_TEXT, 156, COL1_VALUE - COL1_TEXT, 0, &LABEL_FONT, scalestring_cb, NULL, (void *)0);
    GUI_CreateTextSelect(&gui3->scale, COL1_VALUE, 156, TEXTSELECT_96, NULL, set_number100_cb, &mp->cur_mixer->scalar);
    //Row 6
    GUI_CreateLabelBox(&gui3->offsetlbl, COL1_TEXT, 180, COL1_VALUE - COL1_TEXT, 0, &LABEL_FONT, GUI_Localize, NULL, _tr_noop("Offset"));
    GUI_CreateTextSelect(&gui3->offset, COL1_VALUE, 180, TEXTSELECT_96, NULL, set_number100_cb, &mp->cur_mixer->offset);
    GUI_CreateBarGraph(&gui3->bar, COL2_TEXT, 86, 10, 150,
                              CHAN_MIN_VALUE, CHAN_MAX_VALUE, BAR_VERTICAL,
                              eval_chan_cb, NULL);
    GUI_CreateXYGraph(&gui3->graph, COL_GRAPH, 86, 120, 150,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE * 5 / 4,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE * 5 / 4,
                              0, PCT_TO_RANGE(25), eval_mixer_cb, curpos_cb, touch_cb, mp->cur_mixer);
    //Row 7
    GUI_CreateButton(&gui3->trim, COL1_VALUE, 214, BUTTON_96x16, show_trim_cb, toggle_trim_cb, NULL);
    if (! MIXER_SourceHasTrim(MIXER_SRC(mp->mixer[0].src)))
        GUI_SetHidden((guiObject_t *)&gui3->trim, 1);
}

static void _update_rate_widgets(u8 idx)
{
    u8 mix = idx + 1;
    if (MIXER_SRC(mp->mixer[mix].sw)) {
        GUI_SetHidden((guiObject_t *)&gui2->rate[idx], 0);
        if(mp->link_curves & mix) {
            GUI_SetHidden((guiObject_t *)&gui2->linked[idx], 0);
            GUI_SetHidden((guiObject_t *)&gui2->curve[idx], 1);
        } else {
            GUI_SetHidden((guiObject_t *)&gui2->linked[idx], 1);
            GUI_SetHidden((guiObject_t *)&gui2->curve[idx], 0);
        }
        GUI_SetHidden((guiObject_t *)&gui2->scale[idx], 0);
        GUI_SetHidden((guiObject_t *)&gui2->graph[idx], 0);
    } else {
        GUI_SetHidden((guiObject_t *)&gui2->rate[idx], 1);
        GUI_SetHidden((guiObject_t *)&gui2->linked[idx], 1);
        GUI_SetHidden((guiObject_t *)&gui2->curve[idx], 1);
        GUI_SetHidden((guiObject_t *)&gui2->scale[idx], 1);
        GUI_SetHidden((guiObject_t *)&gui2->graph[idx], 1);
    }
}

void MIXPAGE_RedrawGraphs()
{
    switch(mp->cur_template) {
        case MIXERTEMPLATE_SIMPLE:
            GUI_Redraw(&gui1->graph);
            break;
        case MIXERTEMPLATE_EXPO_DR:
            GUI_Redraw(&gui2->graphhi);
            if(OBJ_IS_USED(&gui2->graph[0]))
                GUI_Redraw(&gui2->graph[0]);
            if(OBJ_IS_USED(&gui2->graph[1]))
                GUI_Redraw(&gui2->graph[1]);
            break;
        case MIXERTEMPLATE_COMPLEX:
            GUI_Redraw(&gui3->graph);
            GUI_Redraw(&gui3->bar);
            break;
        default: break;
    }
}

static inline guiObject_t * _get_obj(int idx, int objid)
{
    (void)objid;
    switch(idx) {
        case COMPLEX_TRIM: return (guiObject_t *)&gui3->trim;
        case COMMON_SRC:
            return (guiObject_t *)(mp->cur_template == MIXERTEMPLATE_SIMPLE ? &gui1->src
                                   : mp->cur_template == MIXERTEMPLATE_EXPO_DR ? &gui2->src
                                     : &gui3->src);
        default: return NULL;
    }
}
