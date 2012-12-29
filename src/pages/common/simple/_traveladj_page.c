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

static struct mixer_page * const mp = &pagemem.u.mixer_page;

static const char *travelup_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 ch = (long)data;
    u8 old_scale = Model.limits[ch].servoscale;
    u8 changed = 1;
    Model.limits[ch].servoscale = GUI_TextSelectHelper(Model.limits[ch].servoscale, 1, MAX_TRAVEL_LIMIT, dir, 1, LONG_PRESS_STEP, &changed);
    if (changed && Model.limits[ch].servoscale_neg == 0)
        Model.limits[ch].servoscale_neg = old_scale;  // bug fix: must make sure  scale- won't changed if scale+ is changed
    sprintf(mp->tmpstr, "+%d", Model.limits[ch].servoscale);
    return mp->tmpstr;
}

static const char *traveldown_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 ch = (long)data;
    s16 min = -MAX_TRAVEL_LIMIT;
    s16 value = -Model.limits[ch].servoscale_neg;
    if (value == 0)
        value = -Model.limits[ch].servoscale;
    u8 changed = 1;
    value = GUI_TextSelectHelper(value, min, -1, dir, 1, 5, &changed);
    if (changed) {
        Model.limits[ch].servoscale_neg = -value;
        if (value == Model.limits[ch].servoscale)
            Model.limits[ch].servoscale_neg = 0;
    }
    sprintf(mp->tmpstr, "%d", value);
    return mp->tmpstr;
}
