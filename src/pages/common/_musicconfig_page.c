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

#include "extended_audio.h"

static struct musicconfig_page * const mp = &pagemem.u.musicconfig_page;
static struct musicconfig_obj * const gui = &gui_objs.u.musicconfig;

static u16 current_selected = 0;


const char *musicconfig_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int i = (long)data;
    if ( i < NUM_INPUTS - INP_HAS_CALIBRATION )
        snprintf(tempstring, sizeof(tempstring), _tr("Switch %d"), i + 1);
    else
        snprintf(tempstring, sizeof(tempstring), _tr("Telem %d"),
            i - (NUM_INPUTS - INP_HAS_CALIBRATION) + 1);

    return tempstring;
}

const char *_set_musicsrc_cb(guiTextSelect_t *obj, u8 *src, int dir, int idx, int source)
{
    (void) obj;
    (void) dir;
    (void) idx;
    (void) source;
    u8 changed;

    int newsrc = GUI_TextSelectHelper(MIXER_SRC(*src), INP_HAS_CALIBRATION + 1,
        NUM_SOURCES - NUM_OUT_CHANNELS - NUM_VIRT_CHANNELS - MAX_PPM_IN_CHANNELS, dir, 1, 1, &changed);
    *src = newsrc;
    return INPUT_SourceName(tempstring, *src);
}

const char *musicsrc_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    int idx = (long)data;
    struct CustomMusic *musicpt = &Model.music.custom[idx];
    if (musicpt->src == 0) {
#if HAS_TOUCH
      int cur_row = idx - GUI_ScrollableCurrentRow(&gui->scrollable);
      GUI_SetHidden((guiObject_t *)&gui->idxlbl[cur_row], 1);
      GUI_SetHidden((guiObject_t *)&gui->musicidx[cur_row], 1);
      GUI_SetHidden((guiObject_t *)&gui->musiclbl[cur_row], 1);
#else
      GUI_SetHidden((guiObject_t *)&gui->idxlbl, 1);
      GUI_SetHidden((guiObject_t *)&gui->musicidx, 1);
      GUI_SetHidden((guiObject_t *)&gui->musiclbl, 1);
#endif
      return strcpy(tempstring, _tr("None"));
    }
    if (idx < NUM_INPUTS - INP_HAS_CALIBRATION) {
        const char *str = _set_musicsrc_cb((guiTextSelect_t *)obj, &musicpt->src, dir, idx, INP_NONE);
        return str;
    }
    else {
        musicpt->src = GUI_TextSelectHelper(musicpt->src, TELEM_ALARM_CUSTOM1, TELEM_ALARM_CUSTOM6, dir, 1, 1, NULL);
        snprintf(tempstring, 12, "TELEM%d", musicpt->src - TELEM_CUSTOM_NONE);
        return tempstring;
    }
}

void toggle_musicsrc_cb(guiObject_t *obj, void *data)
{
    int idx = (long)data;
    struct CustomMusic *musicpt = &Model.music.custom[idx];
    if (musicpt->src == 0) {
      if (idx < NUM_INPUTS - INP_HAS_CALIBRATION)
          musicpt->src = INP_HAS_CALIBRATION + 1;
      else
          musicpt->src = TELEM_ALARM_CUSTOM1;
#if HAS_TOUCH
      int cur_row = idx-GUI_ScrollableCurrentRow(&gui->scrollable);
      GUI_SetHidden((guiObject_t *)&gui->idxlbl[cur_row], 0);
      GUI_SetHidden((guiObject_t *)&gui->musicidx[cur_row], 0);
      GUI_SetHidden((guiObject_t *)&gui->musiclbl[cur_row], 0);
#else
      GUI_SetHidden((guiObject_t *)&gui->idxlbl, 0);
      GUI_SetHidden((guiObject_t *)&gui->musicidx, 0);
      GUI_SetHidden((guiObject_t *)&gui->musiclbl, 0);
#endif
    }
    else {
      musicpt->src = 0;
#if HAS_TOUCH
      int cur_row = idx-GUI_ScrollableCurrentRow(&gui->scrollable);
      GUI_SetHidden((guiObject_t *)&gui->idxlbl[cur_row], 1);
      GUI_SetHidden((guiObject_t *)&gui->musicidx[cur_row], 1);
      GUI_SetHidden((guiObject_t *)&gui->musiclbl[cur_row], 1);
#else
      GUI_SetHidden((guiObject_t *)&gui->idxlbl, 1);
      GUI_SetHidden((guiObject_t *)&gui->musicidx, 1);
      GUI_SetHidden((guiObject_t *)&gui->musiclbl, 1);
#endif
    }
    GUI_Redraw(obj);
}

static const char *musiclbl_cb(guiObject_t *obj, const void *data)
{
    (void) obj;
    int idx = (long)data;
    return MUSIC_GetLabel(Model.music.custom[idx].music);
    GUI_Redraw(obj);
}

static void music_test_cb(guiObject_t *obj, void *data)
{
    (void) obj;
    u8 idx = (long)data;
    MUSIC_Play(Model.music.custom[idx].music);
}

static const char *musicid_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    int idx = (long)data;
    struct CustomMusic *musicpt = &Model.music.custom[idx];
    musicpt->music = GUI_TextSelectHelper(musicpt->music, CUSTOM_ALARM_ID, CUSTOM_ALARM_ID + music_map_custom_entries  - 1, dir, 1, 10, NULL);
    snprintf(tempstring, 5, "%d", musicpt->music);
#if HAS_TOUCH
    int cur_row = idx - GUI_ScrollableCurrentRow(&gui->scrollable);
    GUI_Redraw(&gui->musiclbl[cur_row]);
#else
    GUI_Redraw(&gui->musiclbl);
#endif
    return tempstring;
}
