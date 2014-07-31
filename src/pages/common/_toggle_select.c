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

static const struct LabelDesc outline;
static void show_iconsel_page(int idx);
static void tglico_select_cb(guiObject_t *obj, s8 press_type, const void *data);

#ifdef _DEVO12_TARGET_H_
    static char * toggle_files[4] = {
#else
    static const char * const toggle_files[4] = {
#endif
        "media/toggle0.bmp",
        "media/toggle1.bmp",
        "media/toggle2.bmp",
        "media/toggle3.bmp",
    };

static u32 _get_icon_info()
{
    static u32 count = 0; //This gets called from the main page where 'tp' isn't defined
    #ifdef _DEVO12_TARGET_H_
        static u8 checked;
        if(!checked) {
            FILE *fh;
            fh = fopen(toggle_files[3], "r");
            if(!fh)
                toggle_files[3] = "mymedia/toggle3.bmp";
            else
                fclose(fh);
            checked = 1;
        }
    #endif
    if(count == 0) {
        for(int i = 0; i < 4; i++) {
            u16 w, h;
            int ok = LCD_ImageDimensions(toggle_files[i], &w, &h);
            if(ok) {
                //printf("file%d = %d\n", i, w / TOGGLEICON_WIDTH);
                count |= (w / TOGGLEICON_WIDTH) << (i*8);
            }
        }
    }
    return count;
}

struct ImageMap TGLICO_GetImage(int idx)
{
    unsigned int fnum = idx >> 6;
    unsigned int offset = idx & 0x3f;
    u32 count = _get_icon_info();
    if(offset >= ((count >> (8 * fnum)) & 0xff)) {
       offset = 0;
       fnum = 0;
    }
    struct ImageMap img;
    memset(&img, 0, sizeof(img));
    img.file = toggle_files[fnum];
    img.x_off = offset * TOGGLEICON_WIDTH;
    return img;
}

void TGLICO_Select(guiObject_t *obj, const void *data)
{
    (void)obj;
    if(Model.pagecfg2.elem[(long)data].src)
    {
        tp->tglidx = (long)data;
        memcpy(tp->tglicons, Model.pagecfg2.elem[tp->tglidx].extra, sizeof(tp->tglicons));
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
        Model.pagecfg2.elem[tp->tglidx].extra[pos] = 0;
        show_iconsel_page(pos);
    }
}

static int get_toggle_icon_count()
{
    u32 count = _get_icon_info();
    count = ((count >> 24) & 0xff)
            + ((count >> 16) & 0xff)
            + ((count >>  8) & 0xff)
            + ((count >>  0) & 0xff);
    return count;
}

static int get_next_icon(int idx)
{
    u32 count = _get_icon_info();
    unsigned int fnum = idx >> 6;
    unsigned int offset = (idx & 0x3f) + 1;
    while (offset >= ((count >> (fnum * 8)) & 0xff)) {
        fnum++;
        offset = 0;
        if (fnum >= 4)
            return -1;
    }
    return (fnum << 6) | offset;
}



