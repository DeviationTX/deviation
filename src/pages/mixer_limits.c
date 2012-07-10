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

#include "target.h"
#include "pages.h"

static struct mixer_page * const mp = &pagemem.u.mixer_page;
static void sourceselect_cb(guiObject_t *obj, void *data);
static const char *set_source_cb(guiObject_t *obj, int dir, void *data);
static const char *reverse_cb(guiObject_t *obj, int dir, void *data);
static void show_titlerow();
static const char *set_number150_cb(guiObject_t *obj, int dir, void *data);

void MIXPAGE_EditLimits()
{
    GUI_RemoveAllObjects();
    show_titlerow();
    //Row 1
    GUI_CreateLabel(10, 40, NULL, DEFAULT_FONT, "Reverse:");
    GUI_CreateTextSelect(100, 40, TEXTSELECT_96, 0x0000, NULL, reverse_cb, (void *)((long)mp->channel));
    //Row 1
    GUI_CreateLabel(10, 66, NULL, DEFAULT_FONT, "Safety:");
    GUI_CreateTextSelect(70, 66, TEXTSELECT_96, 0x0000, sourceselect_cb, set_source_cb, &mp->limit.safetysw);
    GUI_CreateLabel(170, 66, NULL, DEFAULT_FONT, "Value:");
    GUI_CreateTextSelect(210, 66, TEXTSELECT_96, 0x0000, NULL, PAGEMIX_SetNumberCB, &mp->limit.safetyval);
    //Row 2
    GUI_CreateLabel(10, 92, NULL, DEFAULT_FONT, "Min:");
    GUI_CreateTextSelect(70, 92, TEXTSELECT_96, 0x0000, NULL, set_number150_cb, &mp->limit.min);
    GUI_CreateLabel(170, 92, NULL, DEFAULT_FONT, "Max:");
    GUI_CreateTextSelect(210, 92, TEXTSELECT_96, 0x0000, NULL, set_number150_cb, &mp->limit.max);
}

void sourceselect_cb(guiObject_t *obj, void *data)
{
    u8 *source = (u8 *)data;
    MIX_SET_SRC_INV(*source, ! MIX_SRC_IS_INV(*source));
    GUI_Redraw(obj);
}

const char *set_source_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    u8 *source = (u8 *)data;
    u8 is_neg = MIX_SRC_IS_INV(*source);
    *source = GUI_TextSelectHelper(MIX_SRC(*source), 0, NUM_INPUTS + NUM_CHANNELS, dir, 1, 1, NULL);
    MIX_SET_SRC_INV(*source, is_neg);
    return MIXPAGE_SourceName(*source);
}

const char *set_number150_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    s8 *value = (s8 *)data;
    *value = GUI_TextSelectHelper(*value, -150, 150, dir, 1, 5, NULL);
    sprintf(mp->tmpstr, "%d", *value);
    return mp->tmpstr;
}

static void okcancel_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    if (data) {
        //Save mixer here
        MIX_SetLimit(mp->channel, &mp->limit);
        MIX_SetTemplate(mp->channel, mp->cur_template);
        MIX_SetMixers(mp->mixer, mp->num_mixers);
    }
    GUI_RemoveAllObjects();
    mp->graph = NULL;
    PAGE_MixerInit(0);
}

static void show_titlerow()
{
    GUI_CreateLabel(10, 10, NULL, DEFAULT_FONT, (void *)channel_name[mp->channel]);
    GUI_CreateButton(150, 6, BUTTON_90, "Cancel", 0x0000, okcancel_cb, (void *)0);
    GUI_CreateButton(264, 6, BUTTON_45, "Ok", 0x0000, okcancel_cb, (void *)1);
}

const char *reverse_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)dir;
    (void)data;
    if (dir > 0)
        mp->limit.reverse = 1;
    else if (dir < 0)
        mp->limit.reverse = 0;
    return mp->limit.reverse ? "Inverse" : "Normal";
}
