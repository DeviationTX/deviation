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

#define OVERRIDE_PLACEMENT
#include "common.h"
#include "pages.h"
#include "gui/gui.h"
#include "config/model.h"

#define SEPARATOR 0

enum {
    NUM_COLS      = 7,
    REVERT_X      = LCD_WIDTH - 7*ITEM_SPACE,
    REVERT_W      = 7*ITEM_SPACE ,
    ROW_Y         = 4,
    ROW_INCREMENT = 2,
    LABEL_X       = 0,
    LABEL_W       = 2*ITEM_SPACE,
    LABEL_H       = ITEM_SPACE,
    ICON_X        = 3*ITEM_SPACE,
    ICON_W        = ITEM_SPACE,
    SCROLLABLE_X  = 7*ITEM_SPACE,
    SCROLLABLE_Y  = 4*ITEM_SPACE,
    SCROLLABLE_H  = 8*ITEM_SPACE,
    SCROLL_ROW_H  = 2*ITEM_SPACE,
};

static struct toggleselect_obj   * const gui = &gui_objs.u.toggleselect;
static struct toggle_select_page * const tp  = &pagemem.u.toggle_select_page;

#include "../common/_toggle_select.c"

const char *TGLICO_font_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int idx = (long)data  & 0xff;
    tempstring[0] = 0xc4 | (idx >> 6);
    tempstring[1] = 0x80 | (idx & 0x3f);
    tempstring[2] = 0;
    return tempstring;
}

static int row_cb(int absrow, int relrow, int y, void *data)
{
    int SelectedIcon = (long)data;
    int i;
    int count = get_toggle_icon_count();
    int cols = count - absrow * NUM_COLS;
    if (cols > NUM_COLS)
        cols = NUM_COLS;
    for(i = 0; i < cols; i++) {
        int x = 12 + i * 6;
        GUI_CreateLabelBox(&gui->symbolicon[relrow * NUM_COLS + i], x, y, 4, 2,
                           &DEFAULT_FONT, TGLICO_font_cb, tglico_select_cb, (void*)(long)((SelectedIcon << 8) | (absrow*NUM_COLS+i)));
    }
    return i;
}
#if HAS_MAPPED_GFX
void TGLICO_LoadFonts()
{
    static int loaded = 0;
    if (loaded)
        return;
    loaded = 1;
    int count = get_toggle_icon_count();
    int idx = 0;
    for (int i = 0; i < count; i++) {
        struct ImageMap img;
        img = TGLICO_GetImage(idx);
        LCD_LoadFont(i, img.file, img.x_off, img.y_off, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT);
        idx = get_next_icon(idx);
    }
}
#endif

#include "../128x64x1/toggle_select.c"

#if 0

static unsigned _action_cb(u32 button, unsigned flags, void *data);
static s8 current_toggleicon = 0;

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

    //Load all fonts
    int count = get_toggle_icon_count();
    TGLICO_LoadFonts();
    //Header
    PAGE_ShowHeader(INPUT_SourceNameAbbrevSwitch(tempstring, toggleinput));
    labelDesc.style = LABEL_CENTER;
    w = 14;
    GUI_CreateButtonPlateText(&gui->revert, LCD_WIDTH - w, 0, w, HEADER_WIDGET_HEIGHT, &labelDesc, NULL, 0, revert_cb, (void *)_tr("Revert"));

    labelDesc.style = LABEL_INVERTED;

    //GUI_CreateRect(&gui->separator, 12 + TOGGLEICON_WIDTH, HEADER_WIDGET_HEIGHT, 1, LCD_HEIGHT-HEADER_HEIGHT, &labelDesc);

    int row = 4;
    GUI_CreateLabelBox(&gui->togglelabel[0], 0, row, 4, 2, SelectedIcon == 0 ? &labelDesc : &DEFAULT_FONT, NULL, NULL, "0:");
    img = TGLICO_GetImage(Model.pagecfg2.elem[tp->tglidx].extra[0]);
    GUI_CreateLabelBox(&gui->toggleicon[0], 6, row, 2, 2, &DEFAULT_FONT, TGLICO_font_cb, NULL, (void*)(long)Model.pagecfg2.elem[tp->tglidx].extra[0]); 
    //GUI_CreateImageOffset(&gui->toggleicon[0], 10, row, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT, img.x_off, img.y_off, img.file,
    //         NULL, //SelectedIcon == 0 ? tglico_reset_cb : tglico_setpos_cb,
    //         (void *)0L);

    row += 2;
    GUI_CreateLabelBox(&gui->togglelabel[1], 0, row, 4, 2, SelectedIcon == 1 ? &labelDesc : &DEFAULT_FONT, NULL, NULL, "1:");
    img = TGLICO_GetImage(Model.pagecfg2.elem[tp->tglidx].extra[1]);
    GUI_CreateLabelBox(&gui->toggleicon[1], 6, row, 2, 2, &DEFAULT_FONT, TGLICO_font_cb, NULL, (void *)(long)Model.pagecfg2.elem[tp->tglidx].extra[1]); 
    //GUI_CreateImageOffset(&gui->toggleicon[1], 10, row, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT, img.x_off, img.y_off, img.file,
    //         NULL, //SelectedIcon == 1 ? tglico_reset_cb : tglico_setpos_cb,
    //         (void *)1L);

    if (num_positions == 3) {
        row += 2;
        GUI_CreateLabelBox(&gui->togglelabel[2], 0, row, 4, 2, SelectedIcon == 2 ? &labelDesc : &DEFAULT_FONT, NULL, NULL, "2:");
        img = TGLICO_GetImage(Model.pagecfg2.elem[tp->tglidx].extra[2]);
        GUI_CreateLabelBox(&gui->toggleicon[2], 6, row, 2, 2, &DEFAULT_FONT, TGLICO_font_cb, NULL, (void *)(long)Model.pagecfg2.elem[tp->tglidx].extra[2]); 
        //GUI_CreateImageOffset(&gui->toggleicon[2], 10, row, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT, img.x_off, img.y_off, img.file,
        //     NULL, //SelectedIcon == 2 ? tglico_reset_cb : tglico_setpos_cb,
        //     (void *)2L);
    }

    int rows = (count + NUM_COLS - 1) / NUM_COLS;
    int visible_toggle_rows = (LCD_HEIGHT - HEADER_HEIGHT - 2) / (TOGGLEICON_HEIGHT + 1);
    GUI_CreateScrollable(&gui->scrollable, 10, HEADER_HEIGHT + 2, LCD_WIDTH - 10, LCD_HEIGHT - HEADER_HEIGHT - 2,
                     2, rows, row_cb, getobj_cb, NULL, (void *)(long)SelectedIcon);
    //GUI_SetSelected(GUI_ShowScrollableRowOffset(&gui->scrollable, Model.pagecfg.tglico[tp->tglidx][SelectedIcon]));
}

static void navigate_toggleicons(s8 direction) {
    u8 toggleinput = MIXER_SRC(Model.pagecfg2.elem[tp->tglidx].src);
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
#endif
