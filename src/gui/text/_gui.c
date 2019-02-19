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

static unsigned int _handle_modalbuttons(u32 button, unsigned int flags, void *data);

const struct ImageMap image_map[] = {
    {NULL, 1, 1, 0, 0}
};

void _gui_hilite_selected(struct guiObject *obj)
{
    (void)obj;
}

void _gui_draw_background(int x, int y, int w, int h)
{
    if (w == LCD_WIDTH && h == LCD_HEIGHT)
        LCD_Clear(0x00);
    else
        LCD_FillRect(x, y, w, h, 0x0);  // clear the area
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
                _handle_modalbuttons,
                NULL);
}

static unsigned int _handle_modalbuttons(u32 button, unsigned int flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT) ) {
            objTOUCHED = objModalButton;  // assume the cancel button is the default/modal button
            GUI_TouchRelease();
        } else if (CHAN_ButtonIsPressed(button, BUT_ENTER)) {
            if (objSELECTED)
                objTOUCHED = objSELECTED;
            else
                objTOUCHED = objModalButton;
            GUI_TouchRelease();
        }
        else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

#if HAS_MAPPED_GFX
void _GUI_CreateMappedItem_Helper(guiObject_t *obj)
{
    struct guiBox *box = &obj->box;
    LCD_CreateMappedWindow(1, box->x, box->y, box->width, box->height);
    box->x = 0;
    box->y = 0;
    box->width = box->width * CHAR_WIDTH;
    box->height = box->height * CHAR_HEIGHT;
}

void _GUI_DrawMappedStart()
{
    LCD_SetMappedWindow(1);
}
void _GUI_DrawMappedStop()
{
    LCD_SetMappedWindow(0);
}
#endif
