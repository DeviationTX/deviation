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

static struct toggleselect_obj   * const gui = &gui_objs.u.toggleselect;
static struct toggle_select_page * const tp  = &pagemem.u.toggle_select_page;

static const int NUM_COLS = 8;

#include "../common/_toggle_select.c"

static unsigned _action_cb(u32 button, unsigned flags, void *data);
static s8 current_toggleicon = 0;

static void tglico_select_cb(guiObject_t *obj, s8 press_type, const void *data)
{
    (void)obj;
    if (press_type == -1) {
        // --> data = (ToggleNumber << 12) | (IconNumber << 8) | IconPosition
        u8 IconPosition = ((long)data      ) & 0xff;
        u8 IconNumber   = ((long)data >> 8 ) & 0x0f;
        Model.pagecfg2.elem[tp->tglidx].extra[IconNumber] = IconPosition;
        show_iconsel_page(IconNumber);
    }
}

static void revert_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    memcpy(Model.pagecfg2.elem[tp->tglidx].extra, tp->tglicons, sizeof(tp->tglicons));
    show_iconsel_page(0);
}

static guiObject_t *getobj_cb(int relrow, int col, void *data)
{
    (void)data;
    int idx;
    if (col == -1) {
        idx = relrow * NUM_COLS + NUM_COLS -1;
        if (relrow == 3) {
            while(! OBJ_IS_USED(&gui->symbolicon[idx]))
                idx--;
        }
    } else {
        idx = relrow * NUM_COLS + col;
    }
    return (guiObject_t *)&gui->symbolicon[idx];
}

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
            break;
    }
    return i;
}

static void show_iconsel_page(int SelectedIcon)
{
    struct ImageMap img;
    u16 w;
    PAGE_RemoveAllObjects();
    PAGE_SetActionCB(_action_cb);
    PAGE_SetModal(0);
    memset(gui, 0, sizeof(*gui));
    current_toggleicon = SelectedIcon;
    u8 toggleinput = MIXER_SRC(Model.pagecfg2.elem[tp->tglidx].src);
    int num_positions = INPUT_NumSwitchPos(toggleinput);
    if(num_positions < 2)
        num_positions = 2;

    //Header
    PAGE_ShowHeader(INPUT_SourceNameAbbrevSwitch(tempstring, toggleinput));
    labelDesc.style = LABEL_CENTER;
    w = 50;
    GUI_CreateButtonPlateText(&gui->revert, LCD_WIDTH - w, 0, w, HEADER_WIDGET_HEIGHT, &labelDesc, NULL, 0, revert_cb, (void *)_tr("Revert"));

    labelDesc.style = LABEL_INVERTED;

    GUI_CreateRect(&gui->separator, 12 + TOGGLEICON_WIDTH, HEADER_HEIGHT, 1, LCD_HEIGHT-HEADER_HEIGHT, &labelDesc);

    int row = HEADER_HEIGHT + 5;
    GUI_CreateLabelBox(&gui->togglelabel[0], 0, row, 9, TOGGLEICON_HEIGHT, SelectedIcon == 0 ? &labelDesc : &DEFAULT_FONT, NULL, NULL, "0:");
    img = TGLICO_GetImage(Model.pagecfg2.elem[tp->tglidx].extra[0]);
    GUI_CreateImageOffset(&gui->toggleicon[0], 10, row, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT, img.x_off, img.y_off, img.file,
             NULL, //SelectedIcon == 0 ? tglico_reset_cb : tglico_setpos_cb,
             (void *)0L);

    row += TOGGLEICON_HEIGHT + 2;
    GUI_CreateLabelBox(&gui->togglelabel[1], 0, row, 9, TOGGLEICON_HEIGHT, SelectedIcon == 1 ? &labelDesc : &DEFAULT_FONT, NULL, NULL, "1:");
    img = TGLICO_GetImage(Model.pagecfg2.elem[tp->tglidx].extra[1]);
    GUI_CreateImageOffset(&gui->toggleicon[1], 10, row, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT, img.x_off, img.y_off, img.file,
             NULL, //SelectedIcon == 1 ? tglico_reset_cb : tglico_setpos_cb,
             (void *)1L);

    if (num_positions == 3) {
        row += TOGGLEICON_HEIGHT + 2;
        GUI_CreateLabelBox(&gui->togglelabel[2], 0, row, 9, TOGGLEICON_HEIGHT, SelectedIcon == 2 ? &labelDesc : &DEFAULT_FONT, NULL, NULL, "2:");
        img = TGLICO_GetImage(Model.pagecfg2.elem[tp->tglidx].extra[2]);
        GUI_CreateImageOffset(&gui->toggleicon[2], 10, row, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT, img.x_off, img.y_off, img.file,
             NULL, //SelectedIcon == 2 ? tglico_reset_cb : tglico_setpos_cb,
             (void *)2L);
    }

    int count = get_toggle_icon_count();
    int rows = (count + NUM_COLS - 1) / NUM_COLS;
    int visible_toggle_rows = (LCD_HEIGHT - HEADER_HEIGHT - 2) / (TOGGLEICON_HEIGHT + 1);
    GUI_CreateScrollable(&gui->scrollable, 22, HEADER_HEIGHT + 2, LCD_WIDTH - 22, visible_toggle_rows * (TOGGLEICON_HEIGHT + 1),
                     TOGGLEICON_HEIGHT + 1, rows, row_cb, getobj_cb, NULL, (void *)(long)SelectedIcon);
    //GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, Model.pagecfg.tglico[tp->tglidx][SelectedIcon]));
}

void navigate_toggleicons(s8 direction) {
    u8 toggleinput = MIXER_SRC(Model.pagecfg2.elem[tp->tglidx].src);
    int num_positions = INPUT_NumSwitchPos(toggleinput);
    if(num_positions < 2)
        num_positions = 2;
    current_toggleicon += direction;
    if (current_toggleicon < 0)
        current_toggleicon = num_positions -1;
    else if (current_toggleicon >= num_positions)
        current_toggleicon = 0;
    show_iconsel_page(current_toggleicon);
}

void navigate_symbolicons(s8 direction) {
    guiObject_t *obj = GUI_GetSelected();
    if (direction == -1)
        GUI_SetSelected((guiObject_t *)GUI_GetPrevSelectable(obj));
    else
        GUI_SetSelected((guiObject_t *)GUI_GetNextSelectable(obj));
    return;
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
}

unsigned _action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT))
            PAGE_MainLayoutInit(-1);
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
