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

static const char *reverse_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 ch = (long)data;
    if (ch >= NUM_OUT_CHANNELS) {
        return _tr("None");
    }
    if (dir > 0 && ! (Model.limits[ch].flags & CH_REVERSE)) {
        Model.limits[ch].flags |= CH_REVERSE;
    } else if (dir < 0 && (Model.limits[ch].flags & CH_REVERSE)) {
        Model.limits[ch].flags &= ~CH_REVERSE;
    }
    return (Model.limits[ch].flags & CH_REVERSE) ? _tr("Reversed") : _tr("Normal");
}
