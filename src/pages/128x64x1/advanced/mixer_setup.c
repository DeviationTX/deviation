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
#include "../pages.h"
#include "config/model.h"

enum {
   TYPE_X     = 40,
   TYPE_W     = 50,
   SAVE_X     = LCD_WIDTH - 38,
   SAVE_W     = 38,
   LABEL_X    = 0,
   LABEL_W    = 60,
   TEXTSEL_X  = 0,
   TEXTSEL_W  = 60,
   GRAPH_X    = 77,
   GRAPH_Y    = LCD_HEIGHT - 48,
   GRAPH_W    = 48,
   GRAPH_H    = 48,
   LEFT_VIEW_WIDTH   = 60,
   RIGHT_VIEW_HEIGHT = 48,
   LINES_PER_ROW     = 2,
   UNDERLINE         = 1,
};
#endif //OVERRIDE_PLACEMENT
#include "../../common/advanced/_mixer_setup.c"

static unsigned action_cb(u32 button, unsigned flags, void *data);
static void notify_cb(guiObject_t * obj);

static void _show_titlerow()
{
    PAGE_SetActionCB(action_cb);
    mp->entries_per_page = 2;
    memset(gui, 0, sizeof(*gui));

    labelDesc.style = LABEL_UNDERLINE;
    labelDesc.font_color = labelDesc.fill_color = labelDesc.outline_color = 0xffff;
    GUI_CreateLabelBox(&gui->chan, LABEL_X, 0 , TYPE_X - LABEL_X, HEADER_HEIGHT, &labelDesc,
            MIXPAGE_ChanNameProtoCB, NULL, (void *)((long)mp->cur_mixer->dest));
    labelDesc.style = LABEL_CENTER;
    GUI_CreateTextSelectPlate(&gui->tmpl, TYPE_X, 0,  TYPE_W, HEADER_WIDGET_HEIGHT, &labelDesc, NULL, templatetype_cb, (void *)((long)mp->channel));
    GUI_CreateButtonPlateText(&gui->save, SAVE_X, 0, SAVE_W, HEADER_WIDGET_HEIGHT, &labelDesc, NULL, 0, okcancel_cb, (void *)_tr("Save"));
}


static guiObject_t *simple_getobj_cb(int relrow, int col, void *data)
{
    (void)data;
    (void)col;
    return (guiObject_t *)&gui->value[relrow];
}
enum {
    SIMPLE_OFFSET = COMMON_LAST,
    SIMPLE_LAST,
};

static int simple_row_cb(int absrow, int relrow, int y, void *data)
{
    const char *label = "";
    void *tgl = NULL;
    void *value = NULL;
    void *input_value = NULL;
    data = NULL;
    switch(absrow) {
        case COMMON_SRC:
            label = _tr_noop("Src");
            tgl = sourceselect_cb; value = set_source_cb; data = &mp->mixer[0].src; input_value = set_input_source_cb;
            break;
        case COMMON_CURVE:
            label = _tr_noop("Curve");
            tgl = curveselect_cb; value = set_curvename_cb; data = &mp->mixer[0];
            break;
        case COMMON_SCALE:
            label = _tr_noop("Scale");
            value = set_number100_cb; data = &mp->mixer[0].scalar;
            break;
        case SIMPLE_OFFSET:
            label = _tr_noop("Offset");
            value = set_number100_cb; data = &mp->mixer[0].offset;
            break;
    }
    labelDesc.style = LABEL_LEFT;
    GUI_CreateLabelBox(&gui->label[relrow].lbl, LABEL_X, y, LABEL_W, LINE_HEIGHT, &labelDesc, NULL, NULL, _tr(label));
    labelDesc.style = LABEL_CENTER;
    GUI_CreateTextSourcePlate(&gui->value[relrow].ts, TEXTSEL_X, y + (LINES_PER_ROW - 1) * LINE_SPACE,
                         TEXTSEL_W, LINE_HEIGHT, &labelDesc,
                         tgl, value, input_value, data);
    return 1;
}
static void _show_simple()
{
    GUI_SelectionNotify(NULL);
    GUI_Select1stSelectableObj(); // bug fix: muset reset to 1st selectable item, otherwise ,the focus will be wrong

    int left_side_num_elements = (LCD_HEIGHT - HEADER_HEIGHT) / LINE_SPACE;
    left_side_num_elements = left_side_num_elements - left_side_num_elements%2;
    mp->firstObj = GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LEFT_VIEW_WIDTH + ARROW_WIDTH,
                        left_side_num_elements * LINE_SPACE, LINES_PER_ROW * LINE_SPACE, SIMPLE_LAST, simple_row_cb, simple_getobj_cb, NULL, NULL);

    // The following items are not draw in the logical view;
    GUI_CreateXYGraph(&gui->graph, GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE * 5 / 4,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE * 5 / 4,
                              0, 0, eval_mixer_cb, curpos_cb, touch_cb,
                              &mp->mixer[0]);
    OBJ_SET_USED(&gui->bar, 0);
}

static int complex_size_cb(int absrow, void *data) {
    (void)data;
    return (absrow + COMMON_LAST == COMPLEX_TRIM) ? LINES_PER_ROW : 1;
}

static int complex_row_cb(int absrow, int relrow, int y, void *data)
{
    const char *label = "";
    void *tgl = NULL;
    void *value = NULL;
    void *input_value = NULL;
    data = NULL;
    if (absrow + COMMON_LAST == COMPLEX_TRIM) {
        GUI_CreateButtonPlateText(&gui->value[relrow].but, LABEL_X, y,
            LABEL_W, LINE_HEIGHT, &labelDesc, show_trim_cb, 0x0000, toggle_trim_cb, NULL);
        if (! MIXER_SourceHasTrim(MIXER_SRC(mp->mixer[0].src)))
            GUI_SetHidden((guiObject_t *)&gui->label[relrow], 1);
        return 1;
    }
    switch(absrow + COMMON_LAST) {
        case COMPLEX_MIXER:
            label = _tr_noop("Mixers");
            value = set_nummixers_cb;
            break;
        case COMPLEX_PAGE:
            label = _tr_noop("Page");
            tgl = reorder_cb; value = set_mixernum_cb;
            break;
        case COMPLEX_SWITCH:
            label = _tr_noop("Switch");
            tgl = sourceselect_cb; value = set_drsource_cb; data = &mp->cur_mixer->sw; input_value = set_input_source_cb;
            break;
        case COMPLEX_MUX:
            label = _tr_noop("Mux");
            value = set_mux_cb;
            break;
        case COMPLEX_SRC:
            label = _tr_noop("Src");
            tgl = sourceselect_cb; value = set_source_cb; data = &mp->cur_mixer->src; input_value = set_input_source_cb;
            break;
        case COMPLEX_CURVE:
            label = _tr_noop("Curve");
            tgl = curveselect_cb; value = set_curvename_cb; data = mp->cur_mixer;
            break;
        case COMPLEX_SCALE:
            label = _tr_noop("Scale");
            value = set_number100_cb; data = &mp->cur_mixer->scalar;
            break;
        case COMPLEX_OFFSET:
            label = _tr_noop("Offset");
            value = set_number100_cb; data = &mp->cur_mixer->offset;
            break;
    }
    labelDesc.style = LABEL_LEFT;
    GUI_CreateLabelBox(&gui->label[relrow].lbl, LABEL_X, y, LABEL_W, LINE_HEIGHT,
            &labelDesc, NULL, NULL, _tr(label));
    labelDesc.style = LABEL_CENTER;
    GUI_CreateTextSourcePlate(&gui->value[relrow].ts, TEXTSEL_X, y + (LINES_PER_ROW - 1) * LINE_SPACE,
                         TEXTSEL_W, LINE_HEIGHT, &labelDesc, tgl, value, input_value, data);
    if (absrow + COMMON_LAST == COMPLEX_SRC)
        set_src_enable(CURVE_TYPE(&mp->cur_mixer->curve));
    return 1;
}

static void _show_complex(int page_change)
{
    GUI_SelectionNotify(NULL);
    mp->max_scroll = 2;
    int selection = 0;
    if (page_change) {
        selection = GUI_ScrollableGetObjRowOffset(&gui->scrollable, GUI_GetSelected());
    }
    int left_side_num_elements = (LCD_HEIGHT - HEADER_HEIGHT) / LINE_SPACE;
    left_side_num_elements = left_side_num_elements - left_side_num_elements%2;
    mp->firstObj = GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LEFT_VIEW_WIDTH + ARROW_WIDTH,
                        left_side_num_elements * LINE_SPACE, LINES_PER_ROW * LINE_SPACE, COMPLEX_LAST - COMMON_LAST, complex_row_cb, simple_getobj_cb, complex_size_cb, NULL);

    // The following items are not draw in the logical view;
    GUI_CreateBarGraph(&gui->bar, LEFT_VIEW_WIDTH +10, LCD_HEIGHT - RIGHT_VIEW_HEIGHT -1, 5, RIGHT_VIEW_HEIGHT,
                              CHAN_MIN_VALUE, CHAN_MAX_VALUE, BAR_VERTICAL, eval_chan_cb, NULL);
    GUI_CreateXYGraph(&gui->graph, GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H,
                                  CHAN_MIN_VALUE, CHAN_MIN_VALUE * 5 / 4,
                                  CHAN_MAX_VALUE, CHAN_MAX_VALUE * 5 / 4,
                                  0, 0, eval_mixer_cb, curpos_cb, touch_cb, mp->cur_mixer);
    if (page_change) {
        GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, selection));
    } else {
        GUI_Select1stSelectableObj(); // bug fix: muset reset to 1st selectable item, otherwise ,the focus will be wrong
    }
}

enum {
    EXPO_SWITCH1 = COMMON_LAST,
    EXPO_LINK1,
//    EXPO_CURVE1,
    EXPO_SCALE1,
    EXPO_SWITCH2,
    EXPO_LINK2,
//    EXPO_CURVE2,
    EXPO_SCALE2,
    EXPO_LAST,
};

static guiObject_t *expo_getobj_cb(int relrow, int col, void *data)
{
    (void)data;
    (void)col;
printf("Getobj: 1\n");
    if (col == 1 || ((guiObject_t *)&gui->label[relrow])->Type == Label) {
        return (guiObject_t *)&gui->value[relrow];
    }
    printf("Getobj: label\n");
    return (guiObject_t *)&gui->label[relrow];
}
static int expo_size_cb(int absrow, void *data)
{
    (void)data;
    return LINES_PER_ROW;
    switch(absrow) {
        case EXPO_LINK1:
//        case EXPO_CURVE1:
        case EXPO_LINK2:
//        case EXPO_CURVE2:
            return 1;
    }
    return LINES_PER_ROW;
}
static int expo_row_cb(int absrow, int relrow, int y, void *data)
{
    const char *label = NULL;
    int underline = 0;
    void * label_cb = NULL;
    void *tgl = NULL;
    void *value = NULL;
    void *input_value = NULL;
    data = NULL;
    int but = 0;
    int disable = 0;
    long idx;
    long butidx;
    void *buttgl = NULL;
    void *butdata = NULL;

    switch(absrow) {
        case COMMON_SRC:
            label = _tr("Src");
            tgl = sourceselect_cb; value = set_source_cb; data = &mp->mixer[0].src; input_value = set_input_source_cb;
            break;
        case COMMON_CURVE:
            label = _tr("High-Rate");
            tgl = curveselect_cb; value = set_curvename_cb; data = &mp->mixer[0];
            break;
        case COMMON_SCALE:
            label = (void *)0; label_cb = scalestring_cb;
            value = set_number100_cb; data = &mp->mixer[0].scalar;
            break;
        case EXPO_SWITCH1:
        case EXPO_SWITCH2:
            idx = (absrow == EXPO_SWITCH1) ? 1 : 2;
            label = idx == 1 ? _tr("Switch1") : _tr("Switch2");
            underline = UNDERLINE;
            tgl = sourceselect_cb; value = set_drsource_cb; data = &mp->mixer[idx].sw; input_value = set_input_source_cb;
            break;
        case EXPO_LINK1:
        case EXPO_LINK2:
            butidx = (absrow == EXPO_LINK1) ? 0 : 1;
            buttgl = toggle_link_cb; label_cb = show_rate_cb; butdata = (void *)butidx; but = 1;
            if(! MIXER_SRC(mp->mixer[butidx+1].sw))
                disable = 1;
            idx = butidx+1;
            //break;
        //case EXPO_CURVE1:
        //case EXPO_CURVE2:
            //idx = (absrow == EXPO_CURVE1) ? 1 : 2;
            tgl = curveselect_cb; value = set_curvename_cb; data = &mp->mixer[idx];
            if(! MIXER_SRC(mp->mixer[idx].sw) || mp->link_curves & idx)
                disable = 1;
            break;
        case EXPO_SCALE1:
        case EXPO_SCALE2:
            idx = (absrow == EXPO_SCALE1) ? 1 : 2;
            label = (void *)idx; label_cb = scalestring_cb;
            value = set_number100_cb; data = &mp->mixer[idx].scalar;
            if(! MIXER_SRC(mp->mixer[idx].sw))
                disable = 1;
            break;
    }
    int count = 1;
    if (but) {
        labelDesc.style = LABEL_CENTER;
        GUI_CreateButtonPlateText(&gui->label[relrow].but, LABEL_X, y,
            LABEL_W, LINE_HEIGHT, &labelDesc, label_cb, 0xffff, buttgl, butdata);
        if(disable) {
            GUI_ButtonEnable((guiObject_t *)&gui->label[relrow].but, 0);
        }
        count++;
        y += (LINES_PER_ROW - 1) * LINE_SPACE;
    } else if(label || label_cb) {
        labelDesc.style = LABEL_LEFT;
        GUI_CreateLabelBox(&gui->label[relrow].lbl, LABEL_X, y, LABEL_W, LINE_HEIGHT,
            &labelDesc, label_cb, NULL, label);
        if(underline)
            GUI_CreateRect(&gui->rect1, LABEL_X, y, LABEL_W, 1, &labelDesc);
        y += (LINES_PER_ROW - 1) * LINE_SPACE;
    }
    labelDesc.style = LABEL_CENTER;
    GUI_CreateTextSourcePlate(&gui->value[relrow].ts, TEXTSEL_X, y,
        TEXTSEL_W, LINE_HEIGHT, &labelDesc, tgl, value, input_value, data);
    if(disable) {
        GUI_TextSelectEnable(&gui->value[relrow].ts, 0);
    }
    return count;
}

static void _show_expo_dr()
{
    GUI_SelectionNotify(notify_cb);
    GUI_Select1stSelectableObj();

    sync_mixers();

    int left_side_num_elements = (LCD_HEIGHT - HEADER_HEIGHT) / LINE_SPACE;
    //left_side_num_elements = ((LINES_PER_ROW - 1) + left_side_num_elements) / LINES_PER_ROW;
    left_side_num_elements = left_side_num_elements - left_side_num_elements%2;
    mp->firstObj = GUI_CreateScrollable(&gui->scrollable, 0, HEADER_HEIGHT, LEFT_VIEW_WIDTH + ARROW_WIDTH, 
                        left_side_num_elements * LINE_SPACE, LINE_SPACE, EXPO_LAST, expo_row_cb, expo_getobj_cb, expo_size_cb, NULL);
    
    GUI_CreateXYGraph(&gui->graph, GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H,
                              CHAN_MIN_VALUE, CHAN_MIN_VALUE * 5 / 4,
                              CHAN_MAX_VALUE, CHAN_MAX_VALUE * 5 / 4,
                              0, 0, eval_mixer_cb, curpos_cb, NULL, NULL);

    mp->cur_mixer = &mp->mixer[0];
    //Enable/Disable the relevant widgets
}

static void _update_rate_widgets(u8 idx)
{
    u8 mix = idx + 1;
    guiObject_t *link = GUI_GetScrollableObj(&gui->scrollable, idx ? EXPO_LINK2 : EXPO_LINK1, 0);
    guiObject_t *curve = GUI_GetScrollableObj(&gui->scrollable, idx ? EXPO_LINK2 : EXPO_LINK1, 1);
    guiObject_t *scale = GUI_GetScrollableObj(&gui->scrollable, idx ? EXPO_SCALE2 : EXPO_SCALE1, 0);
    if (MIXER_SRC(mp->mixer[mix].sw)) {
        if(link)
            GUI_ButtonEnable(link, 1);
        if(curve) {
            if(mp->link_curves & mix ) {
                GUI_TextSelectEnable((guiTextSelect_t *)curve, 0);
            } else {
                GUI_TextSelectEnable((guiTextSelect_t *)curve, 1);
            }
        }
        if(scale)
            GUI_TextSelectEnable((guiTextSelect_t *)scale, 1);
    } else {
        if(link)
            GUI_ButtonEnable(link, 0);
        if(curve)
            GUI_TextSelectEnable((guiTextSelect_t *)curve, 0);
        if(scale)
            GUI_TextSelectEnable((guiTextSelect_t *)scale, 0);
    }
}

static void notify_cb(guiObject_t * obj)
{
    if(obj && mp->cur_template == MIXERTEMPLATE_EXPO_DR && OBJ_IS_SCROLLABLE(obj)) {
        /* We exploit the fact that each row has only one selecteable object */
        int row_offset = GUI_ScrollableGetObjRowOffset(&gui->scrollable, obj);
        int idx = (row_offset >> 8) + (row_offset & 0xff);
        if(idx >= EXPO_SWITCH1 && idx <= EXPO_SCALE1) {
            if(mp->cur_mixer != &mp->mixer[1]) {
                mp->cur_mixer = &mp->mixer[1];
                GUI_Redraw(&gui->graph);
            }
        } else if(idx >= EXPO_SWITCH2 && idx <= EXPO_SCALE2) {
            if(mp->cur_mixer != &mp->mixer[2]) {
                mp->cur_mixer = &mp->mixer[2];
                GUI_Redraw(&gui->graph);
            }
        } else {
            if(mp->cur_mixer != &mp->mixer[0]) {
                mp->cur_mixer = &mp->mixer[0];
                GUI_Redraw(&gui->graph);
            }
        }
    }
}

static unsigned action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            GUI_SelectionNotify(NULL);
            PAGE_Pop();
        } else if (CHAN_ButtonIsPressed(button, BUT_ENTER)&& (flags & BUTTON_LONGPRESS)) {
            // long press enter = save without exiting
            PAGE_SaveMixerSetup(mp);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

void MIXPAGE_RedrawGraphs()
{
    switch(mp->cur_template) {
    case MIXERTEMPLATE_COMPLEX:
        GUI_Redraw(&gui->bar);
    case MIXERTEMPLATE_EXPO_DR:
    case MIXERTEMPLATE_SIMPLE:
        GUI_Redraw(&gui->graph);
        break;
    default: break;
    }
}

static inline guiObject_t * _get_obj(int idx, int objid) {
    if (mp->cur_template == MIXERTEMPLATE_COMPLEX) {
        switch(idx) {
            case COMMON_SRC: idx = COMPLEX_SRC; break;
            case COMMON_CURVE: idx = COMPLEX_CURVE; break;
            case COMMON_SCALE: idx = COMPLEX_SCALE; break;
        }
        idx -= COMMON_LAST;
    }
    return GUI_GetScrollableObj(&gui->scrollable, idx, objid);
}
