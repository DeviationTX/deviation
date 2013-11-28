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
static struct Limit origin_limit;

#define gui (&gui_objs.u.advlimit)
static inline guiObject_t *_get_obj(int idx, int objid);
static struct mixer_page * const mp = &pagemem.u.mixer_page;
static void sourceselect_cb(guiObject_t *obj, void *data);
static const char *set_source_cb(guiObject_t *obj, int dir, void *data);
static const char *reverse_cb(guiObject_t *obj, int dir, void *data);
static void toggle_reverse_cb(guiObject_t *obj, void *data);
static void _show_titlerow();
static void _show_limits();
static const char *set_limits_cb(guiObject_t *obj, int dir, void *data);
static const char *set_limitsscale_cb(guiObject_t *obj, int dir, void *data);
static const char *set_trimstep_cb(guiObject_t *obj, int dir, void *data);
static const char *set_failsafe_cb(guiObject_t *obj, int dir, void *data);
static void toggle_failsafe_cb(guiObject_t *obj, void *data);
static void update_safe_val_state();
static const char *set_safeval_cb(guiObject_t *obj, int dir, void *data);

enum {
    ITEM_REVERSE,
    ITEM_FAILSAFE,
    ITEM_SAFETY,
    ITEM_SAFEVAL,
    ITEM_MINLIMIT,
    ITEM_MAXLIMIT,
    ITEM_SCALEPOS,
    ITEM_SCALENEG,
    ITEM_SUBTRIM,
    ITEM_SPEED,
    ITEM_LAST,
};

void MIXPAGE_EditLimits()
{
    PAGE_RemoveAllObjects();
    mp->are_limits_changed = 0;
    memcpy(&origin_limit, (const void *)&mp->limit, sizeof(origin_limit)); // back up for reverting purpose
    _show_titlerow();
    _show_limits();
}

void sourceselect_cb(guiObject_t *obj, void *data)
{
    u8 *source = (u8 *)data;
    if(MIXER_SRC(*source)) {
        mp->are_limits_changed |= 1;
        MIXER_SET_SRC_INV(*source, ! MIXER_SRC_IS_INV(*source));
        GUI_Redraw(obj);
    }
}

static void update_safe_val_state()
{
    guiObject_t *obj = _get_obj(ITEM_SAFEVAL, 0);
    if (!mp->limit.safetysw) {
        if (obj)
            GUI_TextSelectEnable((guiTextSelect_t *)obj, 0);
        mp->limit.safetyval = 0;
    }
    else if (obj)
        GUI_TextSelectEnable((guiTextSelect_t *)obj, 1);
}

const char *set_source_cb(guiObject_t *obj, int dir, void *data)
{
    (void) obj;
    u8 *source = (u8 *)data;
    u8 is_neg = MIXER_SRC_IS_INV(*source);
    u8 isCurrentItemChanged = 0;
    *source = GUI_TextSelectHelper(MIXER_SRC(*source), 0, NUM_SOURCES, dir, 1, 1, &isCurrentItemChanged);
    mp->are_limits_changed |= isCurrentItemChanged;
    update_safe_val_state();  // even there is no change, update_safe_val_state() should still be invoked, otherwise, the revert will fail
    MIXER_SET_SRC_INV(*source, is_neg);
    GUI_TextSelectEnablePress((guiTextSelect_t *)obj, MIXER_SRC(*source));
    return INPUT_SourceName(tempstring, *source);
}

static const char *set_safeval_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    if (!GUI_IsTextSelectEnabled(obj))
        return "0";
    u8 isCurrentItemChanged = 0;
    // bug fix: safe value should be allow to over +/-100
    mp->limit.safetyval = GUI_TextSelectHelper(mp->limit.safetyval, -150, 150, dir, 1, LONG_PRESS_STEP, &isCurrentItemChanged);
    mp->are_limits_changed |= isCurrentItemChanged;
    sprintf(tempstring, "%d", mp->limit.safetyval);
    return tempstring;
}

const char *set_limits_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 *ptr = (u8 *)data;
    int value = *ptr;
    u8 isCurrentItemChanged = 0;
    if (ptr == &mp->limit.min) {
        value = GUI_TextSelectHelper(-value, -250, 0, dir, 1, LONG_PRESS_STEP, &isCurrentItemChanged);
        *ptr = -value;
    } else {
        value = GUI_TextSelectHelper(value, 0, 250, dir, 1, LONG_PRESS_STEP, &isCurrentItemChanged);
        *ptr = value;
    }
    mp->are_limits_changed |= isCurrentItemChanged;
    sprintf(tempstring, "%d", value);
    return tempstring;
}
const char *scalestring_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 idx = (long)data;
    snprintf(tempstring, sizeof(tempstring), _tr("Scale%s"), idx ? "+" : "-");
    return tempstring;
}


const char *set_limitsscale_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 *ptr = (u8 *)data;
    int value = *ptr;
    u8 isCurrentItemChanged = 0;
    if(ptr == &mp->limit.servoscale_neg && *ptr == 0)
        value = mp->limit.servoscale;
    value = GUI_TextSelectHelper(value, 1, 250, dir, 1, LONG_PRESS_STEP, &isCurrentItemChanged);
    if (isCurrentItemChanged) {
        *ptr = value;
        if (ptr == &mp->limit.servoscale) {
            if (mp->limit.servoscale_neg == 0) {
                guiObject_t *obj = _get_obj(ITEM_SCALENEG, 0);
                if(obj)
                    GUI_Redraw(obj);
            }
        } else {
            if (value == mp->limit.servoscale)
                *ptr = 0;
        }
        mp->are_limits_changed = 1;
    }
    sprintf(tempstring, "%d", value);
    return tempstring;
}


const char *set_trimstep_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    s16 *value = (s16 *)data;
    u8 isCurrentItemChanged = 0;
    *value = GUI_TextSelectHelper(*value, -SUBTRIM_RANGE, SUBTRIM_RANGE, dir, 1, LONG_PRESS_STEP, &isCurrentItemChanged);
    mp->are_limits_changed |= isCurrentItemChanged;
    sprintf(tempstring, "%s%d.%d", *value < 0 ? "-" : "", abs(*value) / 10, abs(*value) % 10);
    return tempstring;
}

void toggle_failsafe_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    (void)data;
    mp->are_limits_changed |= 1;
    mp->limit.flags = (mp->limit.flags & CH_FAILSAFE_EN)
          ? (mp->limit.flags & ~CH_FAILSAFE_EN)
          : (mp->limit.flags | CH_FAILSAFE_EN);
}

const char *set_failsafe_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    if (!(mp->limit.flags & CH_FAILSAFE_EN))
        return _tr("Off");
    u8 isCurrentItemChanged = 0;
    // bug fix: failsafe value should be allow to over +/-100
    mp->limit.failsafe = GUI_TextSelectHelper(mp->limit.failsafe, -125, 125, dir, 1, LONG_PRESS_STEP, &isCurrentItemChanged);
    mp->are_limits_changed |= isCurrentItemChanged;
    sprintf(tempstring, "%d", mp->limit.failsafe);
    return tempstring;
}

static void okcancel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    if (data) {
        //Save mixer here
        MIXER_SetLimit(mp->channel, &mp->limit);
    } else {
        memcpy(&mp->limit, (const void *)&origin_limit, sizeof(origin_limit));
        MIXER_SetLimit(mp->channel, &mp->limit);  // save
    }
    PAGE_RemoveAllObjects();
    PAGE_MixerInit(mp->top_channel);
}

const char *reverse_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    if (dir > 0 && ! (mp->limit.flags & CH_REVERSE)) {
        mp->limit.flags |= CH_REVERSE;
        mp->are_limits_changed |= 1;
        GUI_Redraw(&gui->title);  // since changes are saved in live ,we need to redraw the title
    } else if (dir < 0 && (mp->limit.flags & CH_REVERSE)) {
        mp->limit.flags &= ~CH_REVERSE;
        mp->are_limits_changed |= 1;
        GUI_Redraw(&gui->title);  // since changes are saved in live ,we need to redraw the title
    }
    return (mp->limit.flags & CH_REVERSE) ? _tr("Reversed") : _tr("Normal");
}

void toggle_reverse_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    (void)data;
    mp->are_limits_changed |= 1;
    mp->limit.flags = (mp->limit.flags & CH_REVERSE)
          ? (mp->limit.flags & ~CH_REVERSE)
          : (mp->limit.flags | CH_REVERSE);
    GUI_Redraw(&gui->title);  // since changes are saved in live ,we need to redraw the title
}

