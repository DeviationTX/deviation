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

#if HAS_MUSIC_CONFIG
static struct musicconfig_page * const mp = &pagemem.u.musicconfig_page;
static struct musicconfig_obj * const gui = &gui_objs.u.musicconfig;
static u16 current_selected = 0;

const char *musicconfig_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int i = (long)data;
    return _tr("Music");
}
#endif


    
