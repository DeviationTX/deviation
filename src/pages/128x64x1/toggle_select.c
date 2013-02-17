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

#define tp pagemem.u.toggle_select_page
#define gui (&gui_objs.u.toggleselect)

#include "../common/_toggle_select.c"

extern const struct LabelDesc outline;
static u8 _action_cb(u32 button, u8 flags, void *data);
static s8 current_toggleicon = 0;

void tglico_select_cb(guiObject_t *obj, s8 press_type, const void *data)
{
    (void)obj;
    if (press_type == -1) {
        // --> data = (ToggleNumber << 12) | (IconNumber << 8) | IconPosition
        u8 IconPosition = ((long)data      ) & 0xff;
        u8 IconNumber   = ((long)data >> 8 ) & 0x0f;
        Model.pagecfg.tglico[tp.tglidx][IconNumber] = IconPosition;
        show_iconsel_page(IconNumber);
    }
}

static void revert_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    memcpy(Model.pagecfg.tglico[tp.tglidx], tp.tglicons, sizeof(tp.tglicons));
    show_iconsel_page(0);
}

static void show_iconsel_page(int SelectedIcon)
{
    struct ImageMap img;
    u16 w, h;
    PAGE_RemoveAllObjects();
    PAGE_SetActionCB(_action_cb);
    PAGE_SetModal(0);
    memset(gui, 0, sizeof(gui));
    current_toggleicon = SelectedIcon;
    u8 toggleinput = MIXER_SRC(Model.pagecfg.toggle[tp.tglidx]);
    int num_positions = INPUT_NumSwitchPos(toggleinput);
    if(num_positions < 2)
        num_positions = 2;

    //Header
    PAGE_ShowHeader(INPUT_SourceNameAbbrevSwitch(tp.str, toggleinput));
    labelDesc.style = LABEL_CENTER;
    w = 50;
    GUI_CreateButtonPlateText(&gui->revert, LCD_WIDTH - w, 0, w, ITEM_HEIGHT, &labelDesc, NULL, 0, revert_cb, (void *)_tr("Revert"));

    labelDesc.style = LABEL_UNDERLINE;

    GUI_CreateRect(&gui->separator, 12 + TOGGLEICON_WIDTH, ITEM_HEIGHT, 1, LCD_HEIGHT-ITEM_HEIGHT, &labelDesc);

    int row = ITEM_HEIGHT + 5;
    GUI_CreateLabelBox(&gui->togglelabel[0], 0, row, 10, ITEM_HEIGHT, SelectedIcon == 0 ? &labelDesc : &DEFAULT_FONT, NULL, NULL, "0:");
    img = TGLICO_GetImage(Model.pagecfg.tglico[tp.tglidx][0]);
    GUI_CreateImageOffset(&gui->toggleicon[0], 10, row, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT, img.x_off, img.y_off, img.file,
             SelectedIcon == 0 ? tglico_reset_cb : tglico_setpos_cb, (void *)0L);

    row += ITEM_HEIGHT + 2;
    GUI_CreateLabelBox(&gui->togglelabel[1], 0, row, 10, ITEM_HEIGHT, SelectedIcon == 1 ? &labelDesc : &DEFAULT_FONT, NULL, NULL, "1:");
    img = TGLICO_GetImage(Model.pagecfg.tglico[tp.tglidx][1]);
    GUI_CreateImageOffset(&gui->toggleicon[1], 10, row, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT, img.x_off, img.y_off, img.file,
             SelectedIcon == 1 ? tglico_reset_cb : tglico_setpos_cb, (void *)1L);

    if (num_positions == 3) {
        row += ITEM_HEIGHT + 2;
        GUI_CreateLabelBox(&gui->togglelabel[2], 0, row, 10, ITEM_HEIGHT, SelectedIcon == 2 ? &labelDesc : &DEFAULT_FONT, NULL, NULL, "2:");
        img = TGLICO_GetImage(Model.pagecfg.tglico[tp.tglidx][2]);
        GUI_CreateImageOffset(&gui->toggleicon[2], 10, row, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT, img.x_off, img.y_off, img.file,
             SelectedIcon == 2 ? tglico_reset_cb : tglico_setpos_cb, (void *)2L);
    }

    LCD_ImageDimensions(TOGGLE_FILE, &w, &h);
    u8 rows = h / TOGGLEICON_HEIGHT;        // icons in rows
    u8 count = w / TOGGLEICON_WIDTH * rows;        // number of icons
    u8 start_x = 10 + TOGGLEICON_WIDTH + 4;
    u8 pos = 0;
    for(int row = 0; row < 4; row++) {
        int y = ITEM_HEIGHT + 2 + (TOGGLEICON_HEIGHT + 1) * row;
        int x = start_x;
        while(x + TOGGLEICON_WIDTH < LCD_WIDTH && pos < count) {
            img = TGLICO_GetImage(pos);
            GUI_CreateImageOffset(&gui->symbolicon[pos], x, y, TOGGLEICON_WIDTH, TOGGLEICON_HEIGHT,
                                  img.x_off, img.y_off, img.file, tglico_select_cb, (void *)((long)(SelectedIcon << 8) | pos));
            if (pos == Model.pagecfg.tglico[tp.tglidx][SelectedIcon])
               GUI_SetSelected((guiObject_t *)&gui->symbolicon[pos]);
            x += TOGGLEICON_WIDTH + 4;
            pos++;
        }
    }
    if (! GUI_GetSelected()) {
        GUI_SetSelected((guiObject_t *)&gui->symbolicon[0]);
    }
}

void navigate_toggleicons(s8 direction) {
    u8 toggleinput = MIXER_SRC(Model.pagecfg.toggle[tp.tglidx]);
    int num_positions = INPUT_NumSwitchPos(toggleinput);
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
    obj = GUI_GetSelected();
    if (obj == (guiObject_t *)&gui->toggleicon[0] || obj == (guiObject_t *)&gui->toggleicon[1] ||
        obj == (guiObject_t *)&gui->toggleicon[2]) { // skip toggle icon for right/left pressing
        if (direction == -1)
            GUI_SetSelected((guiObject_t *)&gui->revert);
        else {
            GUI_SetSelected((guiObject_t *)&gui->symbolicon[0]);
        }
    }
}

u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT))
            PAGE_MainCfgInit(-1);
        else if (CHAN_ButtonIsPressed(button, BUT_UP))
            navigate_toggleicons(-1);
        else if (CHAN_ButtonIsPressed(button, BUT_DOWN))
            navigate_toggleicons(1);
        else if (CHAN_ButtonIsPressed(button, BUT_RIGHT))
            navigate_symbolicons(-1);
        else if (CHAN_ButtonIsPressed(button, BUT_LEFT))
            navigate_symbolicons(1);
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
