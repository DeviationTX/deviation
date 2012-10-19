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
    {NULL, 32, 15, 0, 0}, /*DRAW_BTN32_15 */
};

void _gui_hilite_selected(struct guiObject *obj)
{
    (void)obj;
}

void _gui_draw_background(int x, int y, int w, int h)
{
    LCD_FillRect(x, y, w, h, 0x0);  // clear the area
}
