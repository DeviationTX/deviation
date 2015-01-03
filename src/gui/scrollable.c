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

#include "common.h"
#define ENABLE_GUIOBJECT
#include "gui.h"
#include "config/display.h"

#if HAS_TOUCH
    static int scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data);
#else
    #define scroll_cb NULL
#endif

static int create_scrollable_objs(guiScrollable_t *scrollable, int row, int offset);

guiObject_t *GUI_CreateScrollable(guiScrollable_t *scrollable, u16 x, u16 y, u16 width, u16 height, u8 row_height, u8 item_count,
     int (*row_cb)(int absrow, int relrow, int x, void *data),
     guiObject_t * (*getobj_cb)(int relrow, int col, void *data),
     int (*size_cb)(int absrow, void *data),
     void *data)
{
    struct guiObject *obj = (guiObject_t *)scrollable;
    struct guiBox    *box = &obj->box;
    CLEAR_OBJ(scrollable);

    scrollable->row_cb = row_cb;
    scrollable->getobj_cb = getobj_cb;
    scrollable->row_height = row_height;
    scrollable->item_count = item_count;
    scrollable->max_visible_rows = (height + row_height / 2) / row_height;
    if (scrollable->max_visible_rows > item_count)
        scrollable->max_visible_rows = item_count;
    scrollable->size_cb = size_cb;
    scrollable->cb_data = data;
    scrollable->head = NULL;
    scrollable->cur_row = 0;

    box->x = x;
    box->y = y;
    box->width = width - ARROW_WIDTH;
    box->height = height;

    obj->Type = Scrollable;
    OBJ_SET_TRANSPARENT(obj, 0);
    OBJ_SET_SELECTABLE(obj, 1); //Scrollables aren't really selectable
    connect_object(obj);

    GUI_CreateScrollbar(&scrollable->scrollbar,
              x + width - ARROW_WIDTH,
              y,
              height,
              item_count,
              obj,
              scroll_cb, scrollable);
    create_scrollable_objs(scrollable, 0, 0);
    //force selection to be current object-if there are no selectable contents
    if (! scrollable->num_selectable)
        objSELECTED = obj;

    return obj;
}

void GUI_DrawScrollable(struct guiObject *obj)
{
    struct guiScrollable *scrollable = (struct guiScrollable *)obj;
    guiObject_t *head = objHEAD;
    objHEAD = scrollable->head;
    GUI_DrawObjects();
    objHEAD = head;
}

extern u8 FullRedraw;
void GUI_RemoveScrollableObjs(struct guiObject *obj)
{
    struct guiScrollable *scrollable = (struct guiScrollable *)obj;
    guiObject_t *head = objHEAD;
    objHEAD = scrollable->head;
    GUI_RemoveAllObjects();
    objHEAD = head;
    scrollable->head = NULL;
    FullRedraw = 1;
}

static int get_selectable_idx(guiScrollable_t *scrollable, guiObject_t *obj)
{
    guiObject_t *head = scrollable->head;
    int id = 0;
    while(head) {
        if(! OBJ_IS_HIDDEN(head) && OBJ_IS_SELECTABLE(head)) {
            if(head == obj)
                return id;
            id++;
        }
        head = head->next;
    }
    return -1;
}

static guiObject_t * set_selectable_idx(guiScrollable_t *scrollable, int idx)
{
    //exclude not completely visible items on last row
    if (idx >= 0 || idx < scrollable->num_selectable) {
        guiObject_t *head = scrollable->head;
        int id = 0;
        while(head) {
            if(! OBJ_IS_HIDDEN(head) && OBJ_IS_SELECTABLE(head)) {
                if(idx == id++)
                    return head;
            }
            head = head->next;
        }
    }
    return (guiObject_t *)scrollable;
}

guiScrollable_t *GUI_FindScrollableParent(guiObject_t *obj) {
    guiObject_t *head = objHEAD;
    while(head) {
        if(head->Type == Scrollable) {
            guiObject_t *s_head = ((guiScrollable_t *)head)->head;
            while(s_head) {
                if(obj == s_head)
                    return (guiScrollable_t *)head;
                s_head=s_head->next;
            }
        }
        head = head->next;
    }
    return NULL;
}

static int adjust_row(guiScrollable_t *scrollable, int target_row, int offset)
{
    //This ensures that the next/prev row is completely visible
    //and is where first_row is kept within a valid range
    int bottom_row = scrollable->cur_row + scrollable->visible_rows + offset;
    int height = scrollable->max_visible_rows;
    int maxrow = scrollable->item_count;
    int target = (target_row < 0) ? 0 : (target_row > maxrow) ? maxrow : target_row;
    int first_row = target, last_row = target;

    while(last_row < maxrow && height > 0) {
        height -= scrollable->size_cb ? scrollable->size_cb(last_row, scrollable->cb_data) : 1;
        last_row++;
    }
    if (height < 0) {
        last_row--;
        if (offset <= 0)
            height += scrollable->size_cb(last_row, scrollable->cb_data);
        else if (last_row < bottom_row) {
            //Restart and work backwards to make all of last row visible
            first_row = ++last_row;
            height = scrollable->max_visible_rows;
        }
    }
    while(first_row && height > 0) {
        height -= scrollable->size_cb ? scrollable->size_cb(first_row-1, scrollable->cb_data) : 1;
        first_row--;
    }
    if (offset <= 0 && first_row - height == target)
        first_row = target;

    //printf("First row %d -> %d \tcount %d\n", target_row, first_row, last_row - first_row);
    return first_row;
}

static int create_scrollable_objs(guiScrollable_t *scrollable, int row, int offset)
{
    int idx = get_selectable_idx(scrollable, objSELECTED);  //save index of selected
    int old_rel_lastrow = scrollable->visible_rows - 1;

    if (! row && offset)
        row = scrollable->cur_row + offset;

    row = adjust_row(scrollable, row, offset);

    old_rel_lastrow -= offset = row - scrollable->cur_row;
    if (scrollable->head && ! offset)
        return idx;

    int rel_row = 0;
    int selectable, num_selectable = 0;
    GUI_RemoveScrollableObjs((guiObject_t *)scrollable);
    guiObject_t *head = objHEAD;
    objHEAD = NULL;
    for(int y = scrollable->header.box.y, bottom = y + scrollable->header.box.height;
        y < bottom && row < scrollable->item_count;
        row++, rel_row++)
    {
        selectable = scrollable->row_cb(row, rel_row, y, scrollable->cb_data);
        y += scrollable->row_height * (scrollable->size_cb ? 
                                       scrollable->size_cb(row, scrollable->cb_data) : 1);
        if (y > bottom + scrollable->row_height - 4)
            break;  //is not selectable because it's not completely visible

        //maintain saved index
        if (offset < 0 && idx != -1) {
            idx += selectable; offset++; }
        if (offset > 0 && rel_row > old_rel_lastrow)
            idx -= selectable;

        num_selectable += selectable;
    }
    if (offset > 0)
        idx += num_selectable - scrollable->num_selectable;
    scrollable->num_selectable = num_selectable;
    scrollable->visible_rows = rel_row;
    scrollable->cur_row = row - rel_row;
    scrollable->head = objHEAD;
    objHEAD = head;
    head = scrollable->head;
    while(head) {
        OBJ_SET_SCROLLABLE(head, 1);
        head = head->next;
    }
    int hidden = (rel_row >= scrollable->item_count) ? 1 : 0;
    int scroll_pos = (2 * scrollable->cur_row + rel_row) / 2;
    if (scrollable->cur_row == 0)
        scroll_pos = 0;
    else if(scrollable->cur_row + rel_row == scrollable->item_count)
        scroll_pos = scrollable->item_count - 1;
    GUI_SetHidden((guiObject_t *)&scrollable->scrollbar, hidden);
    if (! hidden)
        GUI_SetScrollbar(&scrollable->scrollbar, scroll_pos);
    //return saved index of selected
    if (idx >= 0 && idx < num_selectable)
        return idx;
    //else has moved off screen, return -1 or num_selectable
    return idx < 0 ? -1 : num_selectable;
}

#if HAS_TOUCH
int scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data) {
    (void)parent;
    (void)pos;
    guiScrollable_t *scrollable = (guiScrollable_t *)data;
    int row = scrollable->cur_row;
    if (direction > 1)
        row += scrollable->visible_rows;
    else if (direction < -1)
        row -= scrollable->visible_rows;
    else
        row += direction;
    create_scrollable_objs(scrollable, row, 0);
    return -1;
}
#endif

guiObject_t *GUI_ScrollableGetNextSelectable(guiScrollable_t *scrollable, guiObject_t *obj)
{
    int idx = -1;
    if (obj)
        //last selection was in the scrollable
        idx = get_selectable_idx(scrollable, obj) + 1;

    else if (scrollable->num_selectable) {  // <- no wrap-around in Channel/Telemetry monitor
        //scroll to first row and select first selectable
        create_scrollable_objs(scrollable, 0, 0);
        idx = 0;
        goto exit;
    }
    if (idx == -1 || idx == scrollable->num_selectable) {
        //no next selectable, move the scrollbar
        if (scrollable->cur_row < scrollable->item_count - scrollable->visible_rows)
        {
            idx = create_scrollable_objs(scrollable, 0, 1);
            if (idx < scrollable->num_selectable - 1)
                idx++;  //found next (scrolled into view)
            else
                idx = scrollable->num_selectable - 1;
        }
    }
exit:
    //go to next selectable
    if ((idx >= 0 && idx < scrollable->num_selectable)
            || scrollable->cur_row < scrollable->item_count - scrollable->visible_rows)
        return set_selectable_idx(scrollable, idx);

    //go to the next item after the Scrollable
    return NULL;
}

guiObject_t *GUI_ScrollableGetPrevSelectable(guiScrollable_t *scrollable, guiObject_t *obj)
{
    int idx = -1;
    if (obj)
        //last selection was in the scrollable
        idx = get_selectable_idx(scrollable, obj) - 1;

    else if (scrollable->num_selectable) {  // <- no wrap-around in Channel/Telemetry monitor
        //scroll to last row and select last selectable
        create_scrollable_objs(scrollable, scrollable->item_count, 0);
        idx = scrollable->num_selectable - 1;
        goto exit;
    }
    if (idx == -1) {
        //no previous selectable, move the scrollbar
        if (scrollable->cur_row)
        {
            idx = create_scrollable_objs(scrollable, 0, -1);
            if (idx > 0)
                idx--;  //found previous (scrolled into view)
            else
                idx = 0;
        }
    }
exit:
    //go to previous selectable
    if (idx >= 0 || scrollable->cur_row)
        return set_selectable_idx(scrollable, idx);

    //go to the previous item before the Scrollable
    return NULL;
}

guiObject_t *GUI_GetScrollableObj(guiScrollable_t *scrollable, int row, int col)
{
    int relrow = row - scrollable->cur_row;
    if (relrow < 0 || relrow >= scrollable->visible_rows || ! scrollable->getobj_cb)
        return NULL;
    return scrollable->getobj_cb(relrow, col, scrollable->cb_data);
}
guiObject_t *GUI_ShowScrollableRowCol(guiScrollable_t *scrollable, int absrow, int col)
{
    if (absrow < scrollable->cur_row
        || absrow >= scrollable->cur_row + scrollable->visible_rows)
    {
        create_scrollable_objs(scrollable, absrow, 0);
    }
    int relrow = absrow - scrollable->cur_row;
    return scrollable->getobj_cb(relrow, col, scrollable->cb_data);
}

guiObject_t *GUI_ShowScrollableRowOffset(guiScrollable_t *scrollable, int row_idx)
{
    create_scrollable_objs(scrollable, row_idx >> 8, 0);

    //number of rows and/or visible rows can change, ie. added, deleted or model changed
    int idx = row_idx & 0xff;
    if (idx > scrollable->num_selectable -1)
        idx = scrollable->num_selectable -1;

    return set_selectable_idx(scrollable, idx);
}
int GUI_ScrollableGetObjRowOffset(guiScrollable_t *scrollable, guiObject_t *obj)
{
    return (scrollable->cur_row << 8) | get_selectable_idx(scrollable, obj);
}
int GUI_ScrollableCurrentRow(guiScrollable_t *scrollable)
{
    return scrollable->cur_row;
}
int GUI_ScrollableVisibleRows(guiScrollable_t *scrollable)
{
    return scrollable->visible_rows;
}
