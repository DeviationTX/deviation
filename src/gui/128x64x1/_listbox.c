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

#define FILL        0
#define TEXT        0x1
#define SELECT      0x1
#define SELECT_TXT  0x0

void _DrawListbox(struct guiObject *obj, u8 redraw_all)
{
    struct guiListbox *listbox = (struct guiListbox *)obj;
    unsigned int font;
    if (listbox->item_count <= listbox->entries_per_page)
        GUI_SetHidden((guiObject_t *)&listbox->scrollbar, 1);
    else
        GUI_SetHidden((guiObject_t *)&listbox->scrollbar, 0);

    font = listbox->desc.font;
    if (redraw_all) {
        LCD_FillRect(obj->box.x, obj->box.y, obj->box.width, obj->box.height, FILL);
    }
    LCD_SetXY(obj->box.x + 1, obj->box.y + LINE_SPACING - 1);
    if(listbox->selected >= listbox->cur_pos && listbox->selected < listbox->cur_pos + listbox->entries_per_page) {
        // Bug fix: each line of row contains both text and line-spacing, so the height should take line-spacing into account
        if (obj != objSELECTED)  // only draw a box when the listbox is not selected
            LCD_DrawRect(obj->box.x,
                         obj->box.y + (listbox->selected - listbox->cur_pos) * listbox->text_height,
                         obj->box.width, listbox->text_height, SELECT);
        else
            LCD_FillRect(obj->box.x,
                         obj->box.y + (listbox->selected - listbox->cur_pos) * listbox->text_height,
                         obj->box.width, listbox->text_height, SELECT);
    }
    LCD_SetFont(font);
    for(s32 i = 0; i < listbox->entries_per_page; i++) {
        const char *str = listbox->string_cb(i + listbox->cur_pos, listbox->cb_data);
        LCD_SetFontColor(((i + listbox->cur_pos == listbox->selected) && (obj == objSELECTED))? SELECT_TXT : TEXT);
        LCD_PrintStringXY(obj->box.x, obj->box.y + listbox->text_height * i + 1, str);
    }
}
