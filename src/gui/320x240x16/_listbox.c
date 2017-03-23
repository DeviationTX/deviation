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

#define FILL        Display.listbox.bg_color    // RGB888_to_RGB565(0xaa, 0xaa, 0xaa)
#define TEXT        Display.listbox.fg_color    // 0x0000
#define SELECT      Display.listbox.bg_select   // RGB888_to_RGB565(0x44, 0x44, 0x44)
#define SELECT_TXT  Display.listbox.fg_select   // 0xFFFF

void _DrawListbox(struct guiObject *obj, u8 redraw_all)
{
    struct guiListbox *listbox = (struct guiListbox *)obj;
    unsigned int font;
    if (listbox->item_count <= listbox->entries_per_page)
        GUI_SetHidden((guiObject_t *)&listbox->scrollbar, 1);
    else
        GUI_SetHidden((guiObject_t *)&listbox->scrollbar, 0);

    font = Display.listbox.font ? Display.listbox.font : DEFAULT_FONT.font;
    if (redraw_all) {
        LCD_FillRect(obj->box.x, obj->box.y, obj->box.width, obj->box.height, FILL);
    }
    LCD_SetXY(obj->box.x + 5, obj->box.y + LINE_SPACING - 1);
    if(listbox->selected >= listbox->cur_pos && listbox->selected < listbox->cur_pos + listbox->entries_per_page) {
        // Bug fix: each line of row contains both text and line-spacing, so the height should take line-spacing into account
        LCD_FillRect(obj->box.x,
                     obj->box.y + (listbox->selected - listbox->cur_pos) * (listbox->text_height),
                     obj->box.width, listbox->text_height, SELECT);
    }
    LCD_SetFont(font);
    for(s32 i = 0; i < listbox->entries_per_page; i++) {
        const char *str = listbox->string_cb(i + listbox->cur_pos, listbox->cb_data);
        LCD_SetFontColor((i + listbox->cur_pos == listbox->selected) ? SELECT_TXT : TEXT);
        LCD_PrintString(str);
        LCD_PrintString("\n");
    }
}
