/*
 This project is ffree software: you can redistribute it and/or modify
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
    {"media/btn96_24.bmp", 96, 24, 0, 0}, /*FILE_BTN96_24 */
    {"media/btn48_24.bmp", 48, 24, 0, 0}, /*FILE_BTN48_24 */
    {"media/btn96_16.bmp", 96, 16, 0, 0}, /*FILE_BTN96_16 */
    {"media/btn64_16.bmp", 64, 16, 0, 0}, /*FILE_BTN64_16 */
    {"media/btn48_16.bmp", 48, 16, 0, 0}, /*FILE_BTN48_16 */
    {"media/btn32_16.bmp", 32, 16, 0, 0}, /*FILE_BTN32_16 */
    {"media/spin96.bmp",   96, 16, 0, 0}, /*FILE_SPIN96 */
    {"media/spin64.bmp",   64, 16, 0, 0}, /*FILE_SPIN64 */
    {"media/spin32.bmp",   32, 16, 0, 0}, /*FILE_SPIN32 */
    {"media/spinp96.bmp",   96, 16, 0, 0}, /*FILE_SPINPRESS96 */
    {"media/spinp64.bmp",   64, 16, 0, 0}, /*FILE_SPINPRESS64 */
    {"media/spinp32.bmp",   32, 16, 0, 0}, /*FILE_SPINPRESS32 */
    {"media/arrows16.bmp", 16, 16, 0, 0}, /*FILE_ARROW_16_UP */
    {"media/arrows16.bmp", 16, 16, 16, 0}, /*FILE_ARROW_16_DOWN */
    {"media/arrows16.bmp", 16, 16, 32, 0}, /*FILE_ARROW_16_RIGHT */
    {"media/arrows16.bmp", 16, 16, 48, 0}, /*FILE_ARROW_16_LEFT */
};

void _gui_hilite_selected(struct guiObject *obj)
{
    int i;
    for(i = 0; i < Display.select_width; i++)
        LCD_DrawRect(obj->box.x+i, obj->box.y+i, obj->box.width-2*i, obj->box.height-2*i, Display.select_color);
}

void _gui_draw_background(int x, int y, int w, int h)
{
    LCD_DrawWindowedImageFromFile(x, y, "media/devo8.bmp", w, h, x, y);
}

// Bug fix: Unlike devo8, devo10's page always has 1 default selected objects. When a dialog, e.g. saftydialog,
// got poped up, the following statement in handle_buttons() will never get satisfied, the dialog hence is stuck.
// ...
//    if (! objTOUCHED || objTOUCHED == objSELECTED) {
// So the modal buttons handler must separate, hence devo8's modal button handling logic keeps as current
// while have a new logic for devo10
void GUI_HandleModalButtons(u8 enable)
{
    if (! enable)
        BUTTON_UnregisterCallback(&button_modalaction);
    else
        BUTTON_RegisterCallback(&button_modalaction,
                0xFFFFFFFF,
                BUTTON_PRESS | BUTTON_RELEASE | BUTTON_LONGPRESS | BUTTON_PRIORITY,
                handle_buttons,
                NULL);
}
