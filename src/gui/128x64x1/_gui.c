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

static u8 _handle_modalbuttons_devo10(u32 button, u8 flags, void *data);

const struct ImageMap image_map[] = {
    {NULL, 37, 15, 0, 0}, /*DRAW_BTN32_18 */
};

void _gui_hilite_selected(struct guiObject *obj)
{
    (void)obj;
}

void _gui_draw_background(int x, int y, int w, int h)
{
    LCD_FillRect(x, y, w, h, 0x0);  // clear the area
}

void GUI_ViewInit()
{
    views[0].boundary = LOGICAL_VIEW_BOUNDARY;
    views[1].boundary = LOGICAL_VIEW_BOUNDARY + 1000;
    views[2].boundary = LOGICAL_VIEW_BOUNDARY + 2000;
}

// Set coordinate for a view's origin
void GUI_SetupLogicalView(u8 view_id, u16 view_orgin_relativeX, u16 view_orgin_relativeY, u8 width, u8 height,
        u8 view_origin_absoluteX, u8 view_origin_absoluteY)
{
    viewObject_t *viewObj = &views[view_id];
    viewObj->orgin_relativeX  = view_orgin_relativeX;
    viewObj->orgin_relativeY = view_orgin_relativeY;
    viewObj->width = width;
    viewObj->height = height;
    viewObj->origin_absoluteX = view_origin_absoluteX;
    viewObj->origin_absoluteY = view_origin_absoluteY;
}

u16 GUI_MapToLogicalView(u8 view_id, u16 x_or_y)
{
    return views[view_id].boundary + x_or_y;
}

u8 GUI_IsCoordinateInsideLogicalView(u16 *x, u16 *y)
{
    if (!GUI_IsLogicViewCoordinate(*x) || !GUI_IsLogicViewCoordinate(*y))
        return 0;
    u8 view_id = 2;
    if (*x >= views[0].boundary && *x < views[1].boundary && *y>= views[0].boundary && *y < views[1].boundary)
        view_id = 0;
    else if (*x >= views[1].boundary && *x < views[2].boundary && *y>= views[1].boundary && *y < views[2].boundary)
        view_id = 1;

    viewObject_t *viewObj = &views[view_id];
    if (viewObj->orgin_relativeX < 0 || viewObj->orgin_relativeY < 0)
        return 0; // the whole view is hidden ,hence the coordinate won't need to draw
    u16 relative_x = *x - viewObj->boundary;
    if (relative_x < viewObj->orgin_relativeX || relative_x > (viewObj->orgin_relativeX + viewObj->width -1))
        return 0;
    u16 relative_y = *y - viewObj->boundary;
    if (relative_y < viewObj->orgin_relativeY || (relative_y > viewObj->orgin_relativeY + viewObj->height -1))
        return 0;
    *x = relative_x - viewObj->orgin_relativeX + viewObj->origin_absoluteX;
    *y = relative_y - viewObj->orgin_relativeY + viewObj->origin_absoluteY;;
    return 1;
}

u8 GUI_IsObjectInsideCurrentView(u8 view_id, struct guiObject *obj)
{
    viewObject_t *viewObj = &views[view_id];
    if (viewObj->orgin_relativeX < 0 || viewObj->orgin_relativeY < 0)
        return 0; // the whole view is hidden ,hence the coordinate won't need to draw
    struct guiBox *box = &obj->box;
    s16 x = box->x;
    s16 y = box->y;
    if (!GUI_IsLogicViewCoordinate(x) || !GUI_IsLogicViewCoordinate(y))
        return 0;
    u16 relative_x = x - viewObj->boundary;
    if (relative_x < viewObj->orgin_relativeX || relative_x > (viewObj->orgin_relativeX + viewObj->width -1))
        return 0;
    u16 relative_y = y - viewObj->boundary;
    if (relative_y < viewObj->orgin_relativeY || (relative_y > viewObj->orgin_relativeY + viewObj->height -1))
        return 0;

    return 1;
}

s16 GUI_GetLogicalViewOriginRelativeY(u8 view_id)
{
    return views[view_id].orgin_relativeY;
}

void GUI_ScrollLogicalView(u8 view_id, s16 y_offset)
{
    if (y_offset < 0) {
        if (views[view_id].orgin_relativeY < -y_offset)
            views[view_id].orgin_relativeY = -y_offset;
    }
    GUI_SetRelativeOrigin(view_id, views[view_id].orgin_relativeX, views[view_id].orgin_relativeY + y_offset);
}

void GUI_ScrollLogicalViewToObject(u8 view_id, struct guiObject *obj, s8 direction)
{
    viewObject_t *viewObj = &views[view_id];
    if (viewObj->orgin_relativeX < 0 || viewObj->orgin_relativeY < 0)
        return; // the whole view is hidden ,hence the coordinate won't need to draw
    struct guiBox *box = &obj->box;
    s16 relative_y = box->y - viewObj->boundary;
    s16 offset = 0;
    if (direction <0)   {// scroll up
        offset = relative_y - viewObj->orgin_relativeY;
    } else {
        offset = relative_y + box->height - (viewObj->orgin_relativeY + viewObj->height);
    }
    GUI_ScrollLogicalView(view_id, offset);
}

void GUI_SetRelativeOrigin(u8 view_id, s16 new_originX, s16 new_originY)
{
    viewObject_t *viewObj = &views[view_id];
    viewObj->orgin_relativeX = new_originX;
    viewObj->orgin_relativeY = new_originY;
    OBJ_SET_DIRTY(viewObj, 1);  // do not redraw view and its objects directly, just notify GUI_RefreshScreen() to handle it
}

s8 GUI_GetViewId(s16 x, s16 y) {
    if (x >= views[0].boundary && x < views[1].boundary && y>= views[0].boundary && y < views[1].boundary)
        return 0;
    if (x >= views[1].boundary && x < views[2].boundary && y>= views[1].boundary && y < views[2].boundary)
        return 1;
    if (x >= views[2].boundary && y>= views[2].boundary)
        return 2;
    return -1;
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
                _handle_modalbuttons_devo10,
                NULL);
}

static u8 _handle_modalbuttons_devo10(u32 button, u8 flags, void *data)
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

