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

static void show_iconsel_page(int idx);

struct ImageMap TGLICO_GetImage(int idx)
{
    static u16 w = 0;
    if(w == 0) {
        u16 h;
	LCD_ImageDimensions(TOGGLE_FILE, &w, &h);
    }
    struct ImageMap img;
    img.file = TOGGLE_FILE;
    img.x_off = (idx * TOGGLEICON_WIDTH) % w;
    img.y_off = ((idx * TOGGLEICON_WIDTH) / w) * TOGGLEICON_HEIGHT;
    return img;
}

void TGLICO_Select(guiObject_t *obj, const void *data)
{
    (void)obj;
    if(Model.pagecfg.toggle[(long)data])
    {
        tp.tglidx = (long)data;
        memcpy(tp.tglicons, Model.pagecfg.tglico[tp.tglidx], sizeof(tp.tglicons));
        show_iconsel_page(0);
    }
}

void tglico_setpos_cb(guiObject_t *obj, s8 press_type, const void *data)
{
    (void)obj;
    if (press_type == -1) {
        u32 pos = (long)data;
        show_iconsel_page(pos);
    }
}

void tglico_reset_cb(guiObject_t *obj, s8 press_type, const void *data)
{
    (void)obj;
    if (press_type == -1) {
        u32 pos = (long)data;
        Model.pagecfg.tglico[tp.tglidx][pos] = 0;
        show_iconsel_page(pos);
    }
}
