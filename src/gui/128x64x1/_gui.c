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

// Although we can create as many logical views as we want, 2 to 3 views should be sufficient
static s16 _view_orgin_relativeX[] = {0, 0, 0};  // can be negative to indicate the whole view is hidden
static s16 _view_orgin_relativeY[] = {0, 0, 0};  // can be negative to indicate the whole view is hidden
static u16 _view_width[] = {0, 0, 0};
static u16 _view_height[] = {0, 0, 0};
static u16 _view_origin_absoluteX[] = {0, 0, 0};
static u16 _view_origin_absoluteY[] = {0, 0, 0};
static u16 _view_boundary[] = {LOGICAL_VIEW_BOUNDARY, LOGICAL_VIEW_BOUNDARY + 1000, LOGICAL_VIEW_BOUNDARY + 2000};
/**
 * Set coordinate for a view's origin
 */
void GUI_SetupLogicalView(u8 view_id, u16 view_orgin_relativeX, u16 view_orgin_relativeY, u8 width, u8 height,
        u8 view_origin_absoluteX, u8 view_origin_absoluteY) {
    _view_orgin_relativeX[view_id] = view_orgin_relativeX;
    _view_orgin_relativeY[view_id] = view_orgin_relativeY;
    _view_width[view_id] = width;
    _view_height[view_id] = height;
    _view_origin_absoluteX[view_id] = view_origin_absoluteX;
    _view_origin_absoluteY[view_id] = view_origin_absoluteY;
}

u16 GUI_MapToLogicalView(u8 view_id, u16 x_or_y)
{
    return _view_boundary[view_id] + x_or_y;
}

u8 GUI_IsCoordinateInsideLogicalView(u8 view_id, u16 *x, u16 *y)
{
    if (_view_orgin_relativeX[view_id] < 0 || _view_orgin_relativeY[view_id] < 0)
        return 0; // the whole view is hidden ,hence the coordinat won't need to draw
    u16 relative_x = *x - _view_boundary[view_id];
    u16 relative_y = *y - _view_boundary[view_id];
    if (relative_x < _view_orgin_relativeX[view_id] || relative_x > (_view_orgin_relativeX[view_id] + _view_width[view_id] -1)||
        relative_y < _view_orgin_relativeY[view_id] || (relative_y > _view_orgin_relativeY[view_id] + _view_height[view_id]-1))
        return 0;
    *x = relative_x - _view_orgin_relativeX[view_id] + _view_origin_absoluteX[view_id];
    *y = relative_y - _view_orgin_relativeY[view_id] + _view_origin_absoluteY[view_id];
    return 1;
}

u8 GUI_IsObjectInsideCurrentView(u8 view_id, struct guiObject *obj)
{
    if (_view_orgin_relativeX[view_id] < 0 || _view_orgin_relativeY[view_id] < 0)
        return 0; // the whole view is hidden ,hence the coordinat won't need to draw
    struct guiBox *box = &obj->box;
    s16 x = box->x;
    s16 y = box->y;
    if (!GUI_IsLogicViewCoordinate(x) || !GUI_IsLogicViewCoordinate(y))
        return 0;
    u16 relative_x = x - _view_boundary[view_id];
    u16 relative_y = y - _view_boundary[view_id];
    if (relative_x < _view_orgin_relativeX[view_id] || relative_x > (_view_orgin_relativeX[view_id] + _view_width[view_id] -1)||
        relative_y < _view_orgin_relativeY[view_id] || (relative_y > _view_orgin_relativeY[view_id] + _view_height[view_id]-1))
        return 0;
    return 1;
}

s16 GUI_GetLogicalViewOriginRelativeY(u8 view_id)
{
    return _view_orgin_relativeY[view_id];
}

void GUI_ScrollLogicalView(u8 view_id, s16 y_offset)
{
    if (y_offset > 0)
        GUI_SetRelativeOrigin(view_id, _view_orgin_relativeX[view_id], _view_orgin_relativeY[view_id] + y_offset);
    else {
        if (_view_orgin_relativeY[view_id] < -y_offset)
            _view_orgin_relativeY[view_id] = -y_offset;
        GUI_SetRelativeOrigin(view_id, _view_orgin_relativeX[view_id], _view_orgin_relativeY[view_id] + y_offset);
    }
}

void GUI_ScrollLogicalViewToObject(u8 view_id, struct guiObject *obj, s8 direction)
{
    if (_view_orgin_relativeX[view_id] < 0 || _view_orgin_relativeY[view_id] < 0)
        return ; // the whole view is hidden ,hence the coordinat won't need to draw
    struct guiBox *box = &obj->box;
    s16 relative_y = box->y - _view_boundary[view_id];
    s16 offset = 0;
    if (direction <0)   {// scroll up
        offset = relative_y - _view_orgin_relativeY[view_id];
    } else {
        offset = relative_y + box->height - (_view_orgin_relativeY[view_id] + _view_height[view_id]);
    }
    GUI_ScrollLogicalView(view_id, offset);
}

void draw_objects_inview(u8 view_id)
{
    struct guiObject *obj = objHEAD;
    while(obj) {
        if(! OBJ_IS_HIDDEN(obj) && GUI_IsObjectInsideCurrentView(view_id, obj))
        {
            GUI_DrawObject(obj);
        }
        obj = obj->next;
    }
}

void GUI_SetRelativeOrigin(u8 view_id, s16 new_originX, s16 new_originY)
{
    if (new_originX < 0 || new_originY < 0) {
        GUI_DrawBackground(GUI_MapToLogicalView(view_id, _view_orgin_relativeX[view_id]),
            GUI_MapToLogicalView(view_id, _view_orgin_relativeY[view_id]),
            _view_width[view_id], _view_height[view_id]);
        _view_orgin_relativeX[view_id] = new_originX;
        _view_orgin_relativeY[view_id] = new_originY;
        return; // the view is to be hidden ,clear current view
    }
    _view_orgin_relativeX[view_id] = new_originX;
    _view_orgin_relativeY[view_id] = new_originY;
    LCD_FillRect(GUI_MapToLogicalView(view_id, new_originX), GUI_MapToLogicalView(view_id, new_originY),
            _view_width[view_id], _view_height[view_id], 0x0);
    draw_objects_inview(view_id);
}

s8 GUI_GetViewId(u16 x, u16 y) {
    if (x >= _view_boundary[0] && x < _view_boundary[1] && y>= _view_boundary[0] && y < _view_boundary[1])
        return 0;
    if (x >= _view_boundary[1] && x < _view_boundary[2] && y>= _view_boundary[1] && y < _view_boundary[2])
        return 1;
    if (x >= _view_boundary[2] && y>= _view_boundary[2])
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
        if (CHAN_ButtonIsPressed(button, BUT_EXIT) || CHAN_ButtonIsPressed(button, BUT_ENTER)) {
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

