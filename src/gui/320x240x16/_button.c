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

const struct ImageMap image_map[] = {
    {"media/btn96_24" IMG_EXT, 96, 24, 0, 0}, /*FILE_BTN96_24 */
    {"media/btn48_24" IMG_EXT, 48, 24, 0, 0}, /*FILE_BTN48_24 */
    {"media/btn96_16" IMG_EXT, 96, 16, 0, 0}, /*FILE_BTN96_16 */
    {"media/btn64_16" IMG_EXT, 64, 16, 0, 0}, /*FILE_BTN64_16 */
    {"media/btn48_16" IMG_EXT, 48, 16, 0, 0}, /*FILE_BTN48_16 */
    {"media/btn32_16" IMG_EXT, 32, 16, 0, 0}, /*FILE_BTN32_16 */
    {"media/spin192"  IMG_EXT,192, 16, 0, 0}, /*FILE_SPIN192 */
    {"media/spin96"   IMG_EXT, 96, 16, 0, 0}, /*FILE_SPIN96 */
    {"media/spin64"   IMG_EXT, 64, 16, 0, 0}, /*FILE_SPIN64 */
    {"media/spin32"   IMG_EXT, 32, 16, 0, 0}, /*FILE_SPIN32 */
    {"media/spinp96"  IMG_EXT, 96, 16, 0, 0}, /*FILE_SPINPRESS96 */
    {"media/spinp64"  IMG_EXT, 64, 16, 0, 0}, /*FILE_SPINPRESS64 */
    {"media/spinp32"  IMG_EXT, 32, 16, 0, 0}, /*FILE_SPINPRESS32 */
    {"media/arrows16" IMG_EXT, 16, 16, 0, 0}, /*FILE_ARROW_16_UP */
    {"media/arrows16" IMG_EXT, 16, 16, 16, 0}, /*FILE_ARROW_16_DOWN */
    {"media/arrows16" IMG_EXT, 16, 16, 32, 0}, /*FILE_ARROW_16_RIGHT */
    {"media/arrows16" IMG_EXT, 16, 16, 48, 0}, /*FILE_ARROW_16_LEFT */
};

const struct ImageMap *_button_image_map(enum ButtonType type)
{
    switch (type) {
        case BUTTON_96:    return &image_map[FILE_BTN96_24]; break;
        case BUTTON_48:    return &image_map[FILE_BTN48_24]; break;
        case BUTTON_96x16: return &image_map[FILE_BTN96_16]; break;
        case BUTTON_64x16: return &image_map[FILE_BTN64_16]; break;
        case BUTTON_48x16: return &image_map[FILE_BTN48_16]; break;
        case BUTTON_32x16: return &image_map[FILE_BTN32_16]; break;
        default: return NULL;
    }
    return NULL;
}

void _DrawButton(struct guiObject *obj)
{
    struct guiBox *box = &obj->box;
    const char *txt;
    u16 x_off, y_off;
    struct guiButton *button = (struct guiButton *)obj;
    GUI_DrawImageHelper(box->x, box->y, button->image, obj == objTOUCHED ? DRAW_PRESSED : DRAW_NORMAL);
    if (button->strCallback) {
        u16 text_w, text_h;
        LCD_SetFont(BUTTON_FONT.font); //Set Font here so callback can calculate size
        txt = button->strCallback(obj, button->cb_data);
        if (txt) {
            LCD_GetStringDimensions((u8 *) txt, &text_w, &text_h);
            x_off = (box->width - text_w) / 2 + box->x;
            y_off = (box->height - text_h) / 2 + box->y + 1;
            LCD_SetFontColor(BUTTON_FONT.font_color);
            LCD_PrintStringXY(x_off, y_off, txt);
        }
    }
}

