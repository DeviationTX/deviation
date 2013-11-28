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

static const char *subtrim_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 ch = (long)data;
    Model.limits[ch].subtrim = GUI_TextSelectHelper(Model.limits[ch].subtrim,
            -SUBTRIM_RANGE, SUBTRIM_RANGE, dir, 1, LONG_PRESS_STEP, NULL);

    u16 abs_value = abs(Model.limits[ch].subtrim);
    sprintf(tempstring, "%s%d.%d", Model.limits[ch].subtrim < 0 ? "-" : "", abs_value / 10, abs_value % 10);
    return tempstring;
}
