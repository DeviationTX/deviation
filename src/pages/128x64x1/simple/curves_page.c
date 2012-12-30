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
#include "gui/gui.h"
#include "config/model.h"
#include "mixer.h"
#include "mixer_simple.h"
#include "simple.h"
#include "../../common/simple/_curves_page.c"

static u8 _action_cb(u32 button, u8 flags, void *data);
static void show_page(CurvesMode curves_mode, int page);

void PAGE_ThroCurvesInit(int page)
{
    show_page(CURVESMODE_THROTTLE, page);
}

void PAGE_PitCurvesInit(int page)
{
    show_page(CURVESMODE_PITCH, page);
}

static void update_textsel_state()
{
    for (u8 i = 1; i < 8; i++) {
        u8 selectable_bitmap = selectable_bitmaps[curve_mode * 4 + pit_mode];
        GUI_TextSelectEnablePress(mp->itemObj[i], 1);
        if (selectable_bitmap >> (i-1) & 0x01) {
            GUI_TextSelectEnable(mp->itemObj[i], 1);
        } else {
            GUI_TextSelectEnable(mp->itemObj[i], 2);
        }
    }
}

static void press_cb(guiObject_t *obj, void *data)
{
    u8 point_num = (long)data;
    u8 *selectable_bitmap = &selectable_bitmaps[curve_mode * 4 + pit_mode];
    if (*selectable_bitmap >> (point_num-1) & 0x01) {
        GUI_TextSelectEnable(obj, 2);
        *selectable_bitmap &= ~(1 << (point_num-1));
    } else {
        GUI_TextSelectEnable(obj, 1);
        *selectable_bitmap |= 1 << (point_num-1);
    }
}


static void show_page(CurvesMode _curve_mode, int page)
{
    (void)page;
    PAGE_SetActionCB(_action_cb);
    PAGE_SetModal(0);
    PAGE_RemoveAllObjects();
    memset(mp, 0, sizeof(*mp));
    curve_mode = _curve_mode;
    if (curve_mode == CURVESMODE_PITCH)
        SIMPLEMIX_GetMixers(mp->mixer_ptr, mapped_simple_channels.pitch, PITCHMIXER_COUNT);
    else
        SIMPLEMIX_GetMixers(mp->mixer_ptr, mapped_simple_channels.throttle, THROTTLEMIXER_COUNT);
    if (!mp->mixer_ptr[0] || !mp->mixer_ptr[1] || !mp->mixer_ptr[2]) {
        GUI_CreateLabelBox(0, 10, 0, ITEM_HEIGHT, &DEFAULT_FONT, NULL, NULL, "Invalid model ini!");// must be invalid model ini
        return;
    }
    u8 mode_count = 3;
    if (mp->mixer_ptr[3] == NULL)
        pit_hold_state = 0;
    else {
        mode_count = 4;
        pit_hold_state = 1;
    }
    for (u8 i = 0; i < mode_count; i++) {
        if (mp->mixer_ptr[i]->curve.type != CURVE_9POINT) // the 1st version uses 7point curve, need to convert to 13point
            mp->mixer_ptr[i]->curve.type = CURVE_9POINT;
    }

    PAGE_ShowHeader(_tr("Mode"));
    GUI_CreateTextSelectPlate(35, 0, 55, ITEM_HEIGHT, &DEFAULT_FONT, NULL, set_mode_cb, (void *)(long)curve_mode);
    mp->itemObj[9] = GUI_CreateTextSelectPlate(92, 0, 36, ITEM_HEIGHT, &DEFAULT_FONT, NULL, set_holdstate_cb, NULL);
    if (pit_mode != PITTHROMODE_HOLD)
        GUI_SetHidden(mp->itemObj[9], 1);

    u8 y = ITEM_SPACE;
    u8 w1 = 5;
    u8 w2 = 32;
    u8 x = 0;
    u8 height = 9;
    GUI_CreateLabelBox(x, y,  w1, height, &TINY_FONT, NULL, NULL, "L");
    x += w1;
    mp->itemObj[0] = GUI_CreateTextSelectPlate(x, y, w2, height, &TINY_FONT, NULL, set_pointval_cb, (void *)(long)0);
    x += w2 + 2;
    GUI_CreateLabelBox(x, y,  w1, height, &TINY_FONT, NULL, NULL, "H");
    x += w1;
    mp->itemObj[8] = GUI_CreateTextSelectPlate(x, y, w2, height, &TINY_FONT, NULL, set_pointval_cb, (void *)(long)8);

    y += height;
    x = 0;
    GUI_CreateButtonPlateText(x, y, 38, ITEM_HEIGHT, &DEFAULT_FONT, NULL, 0, auto_generate_cb, _tr("Auto"));
    x += 39;
    GUI_CreateLabelBox(x, y + 3,  w1, height, &TINY_FONT, NULL, NULL, "M");
    x += w1;
    mp->itemObj[4] = GUI_CreateTextSelectPlate(x, y +3, w2, height, &TINY_FONT, press_cb, set_pointval_cb, (void *)(long)4);

    y += ITEM_SPACE;
    x = 0;
    GUI_CreateLabelBox(x, y,  w1, height, &TINY_FONT, NULL, NULL, "2");
    x += w1;
    mp->itemObj[1] = GUI_CreateTextSelectPlate(x, y, w2, height, &TINY_FONT, press_cb, set_pointval_cb, (void *)(long)1);
    x += w2 + 2;
    GUI_CreateLabelBox(x, y,  w1, height, &TINY_FONT, NULL, NULL, "3");
    x += w1;
    mp->itemObj[2] = GUI_CreateTextSelectPlate(x, y, w2, height, &TINY_FONT, press_cb, set_pointval_cb, (void *)(long)2);

    y += height +1;
    x = 0;
    GUI_CreateLabelBox(x, y,  w1, height, &TINY_FONT, NULL, NULL, "4");
    x += w1;
    mp->itemObj[3] = GUI_CreateTextSelectPlate(x, y, w2, height, &TINY_FONT, press_cb, set_pointval_cb, (void *)(long)3);
    x += w2 + 2;
    GUI_CreateLabelBox(x, y,  w1, height, &TINY_FONT, NULL, NULL, "6");
    x += w1;
    mp->itemObj[5] = GUI_CreateTextSelectPlate(x, y, w2, height, &TINY_FONT, press_cb, set_pointval_cb, (void *)(long)5);

    y += height +1;
    x = 0;
    GUI_CreateLabelBox(x, y,  w1, height, &TINY_FONT, NULL, NULL, "7");
    x += w1;
    mp->itemObj[6] = GUI_CreateTextSelectPlate(x, y, w2, height, &TINY_FONT, press_cb, set_pointval_cb, (void *)(long)6);
    x += w2 + 2;
    GUI_CreateLabelBox(x, y,  w1, height, &TINY_FONT, NULL, NULL, "8");
    x += w1;
    mp->itemObj[7] = GUI_CreateTextSelectPlate(x, y, w2, height, &TINY_FONT, press_cb, set_pointval_cb, (void *)(long)7);

    update_textsel_state();

    mp->graphs[0] = GUI_CreateXYGraph(77, ITEM_HEIGHT +1, 50, 50,
                  CHAN_MIN_VALUE, CHAN_MIN_VALUE,
                  CHAN_MAX_VALUE, CHAN_MAX_VALUE,
                  0, 0, //CHAN_MAX_VALUE / 4, CHAN_MAX_VALUE / 4,
                  show_curve_cb, curpos_cb, NULL, NULL);

    GUI_Select1stSelectableObj();
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    //u8 total_items = 2;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByID(PAGEID_MENU, PREVIOUS_ITEM);
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}
