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

void toggle_failsafe_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    u8 ch = (long)data;
    if (ch >= NUM_OUT_CHANNELS) {
        return;
    }
    Model.limits[ch].flags = (Model.limits[ch].flags & CH_FAILSAFE_EN)
          ? (Model.limits[ch].flags & ~CH_FAILSAFE_EN)
          : (Model.limits[ch].flags | CH_FAILSAFE_EN);
}

const char *set_failsafe_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 ch = (long)data;
    if (ch >= NUM_OUT_CHANNELS) {
        return _tr("None");
    }
    if (!(Model.limits[ch].flags & CH_FAILSAFE_EN))
        return _tr("Off");
    const char *str = PAGEMIXER_SetNumberCB(obj, dir, &(Model.limits[ch].failsafe));
    return str;
}
