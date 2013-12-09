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

static struct toggle_obj * const gui = &gui_objs.u.toggle;
#define tp pagemem.u.toggle_select_page

#include "../common/_toggle_select.c"

static void show_iconsel_page(int idx);
static void show_icons(int SelectedIcon, int idx);
extern void PAGE_MainLayoutRestoreDialog(int);

static const struct LabelDesc outline = {0, 0, 0, 0, LABEL_TRANSPARENT};

void tglico_select_cb(guiObject_t *obj, s8 press_type, const void *data)
{
    (void)obj;
    if (press_type == -1) {
        // --> data = (ToggleNumber << 12) | (IconNumber << 8) | IconPosition
        u8 IconPosition = ((long)data      ) & 0xff;
        u8 IconNumber   = ((long)data >> 8 ) & 0x0f;
        Model.pagecfg2.elem[tp.tglidx].extra[IconNumber] = IconPosition;
        show_iconsel_page(IconNumber);
    }
}

static void tglico_cancel_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    memcpy(Model.pagecfg2.elem[tp.tglidx].extra, tp.tglicons, sizeof(tp.tglicons));
    PAGE_MainLayoutRestoreDialog(tp.tglidx);
}

static void tglico_ok_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    PAGE_MainLayoutRestoreDialog(tp.tglidx);
}

static void show_icons(int SelectedIcon, int idx)
{
    int x, y;
    struct ImageMap img;
    u8 cursel = Model.pagecfg2.elem[tp.tglidx].extra[SelectedIcon];

    for(int i = 0; i < NUM_SYMBOL_ELEMS; i++) {
        y = 80 + (i / NUM_SYMBOL_COLS) * 40;
        x = 8 + (i - (i / NUM_SYMBOL_COLS) * NUM_SYMBOL_COLS) * (TOGGLEICON_WIDTH+8);
        img = TGLICO_GetImage(idx);
        //printf("%d(%d): (%d, %d) %s / %d\n", i, idx, x, y, img.file, img.x_off);
        GUI_CreateImageOffset(&gui->symbolicon[i], x, y, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT,
                              img.x_off, img.y_off, img.file, tglico_select_cb,
                              (void *)((long)(SelectedIcon << 8) | idx));
        if (idx == cursel)
            GUI_CreateRect(&gui->symbolframe, x-1, y-1, TOGGLEICON_WIDTH+2, TOGGLEICON_HEIGHT+2, &outline);
        idx = get_next_icon(idx);
        if(idx < 0)
            break;
    }
}

static int scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data)
{
    (void)data;
    (void)parent;
    int page = pos + (direction > 0 ? 1 : -1);
    if (page < 0) {
        return pos;
    } else if(page >= GUI_GetScrollbarNumItems(&gui->scrollbar)) {
        return pos;
    }
    int idx = 0;
    if (page) {
        for(int i = 0; i < NUM_SYMBOL_COLS * page; i++) {
            idx = get_next_icon(idx);
            if (idx < 0) {
                idx = 0;
                break;
            }
        }
    }
    GUI_RemoveHierObjects((guiObject_t *)&gui->symbolicon[0]);
    show_icons((long)data, idx);
    return page;
}

static void show_iconsel_page(int SelectedIcon)
{
    struct ImageMap img;
    PAGE_RemoveAllObjects();
    PAGE_SetModal(1);
    PAGE_CreateCancelButton(LCD_WIDTH-104, 4, tglico_cancel_cb);
    // Ok-Button for saving
    PAGE_CreateOkButton(LCD_WIDTH-155, 4, tglico_ok_cb);
    // Show name of source for toggle icon
    u8 toggleinput = MIXER_SRC(Model.pagecfg2.elem[tp.tglidx].src);

    // style the switch textbox
    struct LabelDesc outline = { DEFAULT_FONT.font, 0, 0, DEFAULT_FONT.font_color, LABEL_TRANSPARENT };
    GUI_CreateRect(&gui->toggleframe, 80+80*SelectedIcon, 39, 77, 33, &outline);

    GUI_CreateLabelBox(&gui->switchbox,  4, 47, 70, 22, &NORMALBOX_FONT, NULL, NULL,
                       INPUT_SourceNameAbbrevSwitch(tempstring, Model.pagecfg2.elem[tp.tglidx].src));

    int num_positions = INPUT_NumSwitchPos(toggleinput);
    if(num_positions < 2)
        num_positions = 2;

    GUI_CreateLabel(&gui->togglelabel[0], 94, 50, NULL, DEFAULT_FONT, _tr("Pos 0"));
    img = TGLICO_GetImage(Model.pagecfg2.elem[tp.tglidx].extra[0]);
    GUI_CreateImageOffset(&gui->toggleicon[0], 124, 40, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT, img.x_off, img.y_off, img.file,
             SelectedIcon == 0 ? tglico_reset_cb : tglico_setpos_cb, (void *)0L);

    GUI_CreateLabel(&gui->togglelabel[1], 174, 50, NULL, DEFAULT_FONT, _tr("Pos 1"));
    img = TGLICO_GetImage(Model.pagecfg2.elem[tp.tglidx].extra[1]);
    GUI_CreateImageOffset(&gui->toggleicon[1], 204, 40, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT, img.x_off, img.y_off, img.file,
             SelectedIcon == 1 ? tglico_reset_cb : tglico_setpos_cb, (void *)1L);
    if (num_positions == 3) {
        GUI_CreateLabel(&gui->togglelabel[2], 254, 50, NULL, DEFAULT_FONT, _tr("Pos 2"));
        img = TGLICO_GetImage(Model.pagecfg2.elem[tp.tglidx].extra[2]);
        GUI_CreateImageOffset(&gui->toggleicon[2], 284, 40, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT, img.x_off, img.y_off, img.file,
             SelectedIcon == 2 ? tglico_reset_cb : tglico_setpos_cb, (void *)2L);
    }

    int count = get_toggle_icon_count();
    int max_scroll = (count + NUM_SYMBOL_COLS - 1) / NUM_SYMBOL_COLS - (NUM_SYMBOL_ROWS - 1);
    if (max_scroll > 1) GUI_CreateScrollbar(&gui->scrollbar, LCD_WIDTH-16, 80, LCD_HEIGHT-80, max_scroll, NULL, scroll_cb, (void *)(long)SelectedIcon);
    show_icons(SelectedIcon, 0);
}
