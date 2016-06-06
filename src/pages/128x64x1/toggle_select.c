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

#define SEPARATOR 1
enum {
    NUM_COLS      = 8,
    REVERT_X      = LCD_WIDTH - 50,
    REVERT_W      = 50,
    SEPARATOR_X   = 12 + TOGGLEICON_WIDTH,
    #define ROW_Y   (HEADER_HEIGHT + 5)
    ROW_INCREMENT = TOGGLEICON_HEIGHT + 2,
    LABEL_X       = 0,
    LABEL_W       = 9,
    LABEL_H       = TOGGLEICON_HEIGHT,
    ICON_X        = 10,
    ICON_W        = TOGGLEICON_WIDTH,
    SCROLLABLE_X  = 22,
    #define SCROLLABLE_Y (HEADER_HEIGHT + 1)
    #define SCROLL_ROW_H (TOGGLEICON_HEIGHT + 1)
    #define SCROLLABLE_H (((LCD_HEIGHT - HEADER_HEIGHT - 2) / SCROLL_ROW_H) * SCROLL_ROW_H)
};

static struct toggleselect_obj   * const gui = &gui_objs.u.toggleselect;
static struct toggle_select_page * const tp  = &pagemem.u.toggle_select_page;

#include "../common/_toggle_select.c"

static int row_cb(int absrow, int relrow, int y, void *data)
{
    int SelectedIcon = (long)data;
    struct ImageMap img;
    int idx = 0;
    int i;
    for(i = 0; i < NUM_COLS * absrow; i++) {
        idx = get_next_icon(idx);
        if (idx < 0) {
            return 0;
        }
    }
    for(i = 0; i < NUM_COLS; i++) {
        img = TGLICO_GetImage(idx);
        int x = 22 + i * (TOGGLEICON_WIDTH+4);
        GUI_CreateImageOffset(&gui->symbolicon[relrow * NUM_COLS + i], x, y, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT,
                              img.x_off, img.y_off, img.file, tglico_select_cb,
                              (void *)((long)(SelectedIcon << 8) | idx));
        idx = get_next_icon(idx);
        if(idx < 0)
            return i+1;
    }
    return i;
}

#endif //OVERRIDE_PLACEMENT

static unsigned _action_cb(u32 button, unsigned flags, void *data);
static s8 current_toggleicon = 0;

static void show_iconsel_page(int SelectedIcon) {
    GUI_RemoveAllObjects();
    memset(gui, 0, sizeof(*gui));
    current_toggleicon = SelectedIcon;
    int toggleinput = MIXER_SRC(Model.pagecfg2.elem[tp->tglidx].src);

    //Header
    PAGE_ShowHeader(INPUT_SourceNameAbbrevSwitch(tempstring, toggleinput));
    labelDesc.style = LABEL_CENTER;
    GUI_CreateButtonPlateText(&gui->revert, REVERT_X, 0, REVERT_W, HEADER_WIDGET_HEIGHT, &labelDesc, NULL, 0, revert_cb, (void *)_tr("Revert"));

#if SEPARATOR
    GUI_CreateRect(&gui->separator, SEPARATOR_X, HEADER_WIDGET_HEIGHT, 1, LCD_HEIGHT-HEADER_HEIGHT, &labelDesc);
#endif

    int row = ROW_Y;
    int num_positions = INPUT_NumSwitchPos(toggleinput);
    if(num_positions < 2)
        num_positions = 2;

    static const char * const tglidx[3] = {"0:", "1:", "2:"};
    labelDesc.style = LABEL_INVERTED;
    for (int i = 0; i < num_positions; i++) {
        GUI_CreateLabelBox(&gui->togglelabel[i], LABEL_X, row, LABEL_W, LABEL_H, SelectedIcon == i ? &labelDesc : &DEFAULT_FONT, NULL, NULL, tglidx[i]);
#ifdef HAS_CHAR_ICONS
        GUI_CreateLabelBox(&gui->toggleicon[i], ICON_X, row, ICON_W, LABEL_H, &DEFAULT_FONT, TGLICO_font_cb,
                           NULL, (void *)(long)Model.pagecfg2.elem[tp->tglidx].extra[i]); 
#else
        struct ImageMap img = TGLICO_GetImage(Model.pagecfg2.elem[tp->tglidx].extra[i]);
        GUI_CreateImageOffset(&gui->toggleicon[i], ICON_X, row, ICON_W, LABEL_H, img.x_off, img.y_off, img.file,
             NULL, //SelectedIcon == 0 ? tglico_reset_cb : tglico_setpos_cb,
             (void *)(long)i);
#endif

        row += ROW_INCREMENT;
    }

    int count = get_toggle_icon_count();
    int rows = (count + NUM_COLS - 1) / NUM_COLS;
    GUI_CreateScrollable(&gui->scrollable, SCROLLABLE_X, SCROLLABLE_Y, LCD_WIDTH - SCROLLABLE_X,
                     SCROLLABLE_H,
                     SCROLL_ROW_H, rows, row_cb, NULL, NULL, (void *)(long)SelectedIcon);
}

void PAGE_ToggleEditInit(int page)
{
    tp->tglidx = page;
    memcpy(tp->tglicons, Model.pagecfg2.elem[tp->tglidx].extra, sizeof(tp->tglicons));
    PAGE_SetActionCB(_action_cb);
    show_iconsel_page(0);
}

static void navigate_toggleicons(s8 direction) {
    int toggleinput = MIXER_SRC(Model.pagecfg2.elem[tp->tglidx].src);
    int num_positions = INPUT_NumSwitchPos(toggleinput);
    if(num_positions < 2)
        num_positions = 2;
    current_toggleicon += direction;
    if (current_toggleicon < 0)
        current_toggleicon = num_positions;
    else if (current_toggleicon > num_positions)
        current_toggleicon = 0;
    show_iconsel_page(current_toggleicon);
}

static void navigate_symbolicons(s8 direction) {
    guiObject_t *obj = GUI_GetSelected();
    if (direction == -1)
        GUI_SetSelected((guiObject_t *)GUI_GetPrevSelectable(obj));
    else
        GUI_SetSelected((guiObject_t *)GUI_GetNextSelectable(obj));
    return;
#if 0
    obj = GUI_GetSelected();
    if (obj == (guiObject_t *)&gui->toggleicon[0]
        || obj == (guiObject_t *)&gui->toggleicon[1]
        || obj == (guiObject_t *)&gui->toggleicon[2])
    {
        // skip toggle icon for right/left pressing
        if (direction == -1)
            GUI_SetSelected((guiObject_t *)&gui->revert);
        else {
            GUI_SetSelected((guiObject_t *)&gui->symbolicon[0]);
        }
    }
#endif
}

unsigned _action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;
    if (CHAN_ButtonIsPressed(button, BUT_ENTER))
        return 0;
    if (! (flags & BUTTON_RELEASE))
        return 1;
    {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT))
            PAGE_Pop();
        else if (CHAN_ButtonIsPressed(button, BUT_UP))
            navigate_toggleicons(-1);
        else if (CHAN_ButtonIsPressed(button, BUT_DOWN))
            navigate_toggleicons(1);
        else if (CHAN_ButtonIsPressed(button, BUT_LEFT))
            navigate_symbolicons(-1);
        else if (CHAN_ButtonIsPressed(button, BUT_RIGHT))
            navigate_symbolicons(1);
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
