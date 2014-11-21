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
    struct guiBox    *box;
    CLEAR_OBJ(scrollable);

    box = &obj->box;
    scrollable->row_cb = row_cb;
    scrollable->getobj_cb = getobj_cb;
    scrollable->row_height = row_height;
    scrollable->item_count = item_count;
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
    scrollable->max_visible_rows = (height + row_height / 2) / row_height;
    if (scrollable->max_visible_rows > item_count)
        scrollable->max_visible_rows = item_count;
    
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
    int found = -1;
    int id = 0;
    while(head) {
        if(! OBJ_IS_HIDDEN(head) && OBJ_IS_SELECTABLE(head)) {
            if(head == obj)
                found = id;
            id++;
        }
        head = head->next;
    }
    return found;
}

static guiObject_t * set_selectable_idx(guiScrollable_t *scrollable, int idx)
{
    guiObject_t *head = scrollable->head;
    int id = 0;
    while(head) {
        if(! OBJ_IS_HIDDEN(head) && OBJ_IS_SELECTABLE(head)) {
            if(idx == id++)
                return head;
        }
        head = head->next;
    }
    return NULL;
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

static int adjust_row(guiScrollable_t *scrollable, int row, int offset)
{
    int height = scrollable->max_visible_rows;
    int max_firstrow = scrollable->item_count - height;
    if (row < 0) row = 0;
    if (! scrollable->size_cb)
        return row > max_firstrow ? max_firstrow : row;

    max_firstrow = scrollable->item_count - 1;
    if (row > max_firstrow)
        row = max_firstrow;

    //This ensures that the next/prev row is completely visible
    int first_row = row, count = 0; //int orig_row = row;
    while(row <= max_firstrow && height) {
        height -= scrollable->size_cb(row, scrollable->cb_data);
        if (height < 0)
            break;
        row++;
        count++;
    }
    if (height < 0 && offset > 0) {
        //Restart and work backwards to make all of last row visible
        height = scrollable->max_visible_rows;
        first_row = ++row;
        count = 0;
    }
    while(first_row) {
        height -= scrollable->size_cb(first_row-1, scrollable->cb_data);
        if (height < 0)
            break;
        first_row--;
        count++;
    }
    scrollable->visible_rows = count;
    //printf("First row %d -> %d\n", orig_row, first_row);
    return first_row;
}

static int create_scrollable_objs(guiScrollable_t *scrollable, int row, int offset)
{
    int idx = get_selectable_idx(scrollable, objSELECTED);  //save index of selected
    int old_rel_lastrow = scrollable->visible_rows - 1;

    //adjust row and calculate visible_rows
    if (row || ! offset) {
        row = adjust_row(scrollable, row, offset);
    } else {
        row = adjust_row(scrollable, scrollable->cur_row + offset, offset);
    }
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
    if (rel_row >= scrollable->item_count)
        GUI_SetHidden((guiObject_t *)&scrollable->scrollbar, 1);
    else {
        int scroll_pos = (2 * scrollable->cur_row + rel_row) / 2;
        if (scrollable->cur_row == 0)
            scroll_pos = 0;
        else if(scrollable->cur_row + rel_row == scrollable->item_count)
            scroll_pos = scrollable->item_count - 1;
        GUI_SetScrollbar(&scrollable->scrollbar, scroll_pos);
    }
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
    int idx = -2;
    if (obj)
        //last selection was in the scrollable
        idx = get_selectable_idx(scrollable, obj);

    //first handle any required scrolling, ...selectables can scroll into view
    if ((idx == scrollable->num_selectable -1 || ! scrollable->num_selectable)
            && scrollable->cur_row < scrollable->item_count - scrollable->visible_rows)
        //no next selectable (at last item, or no selectable)
        //and the last row is not visible, just move the scrollbar
        idx = create_scrollable_objs(scrollable, 0, 1);

    else if (idx == -2 && scrollable->num_selectable) { //if no selectables, no wrap-around
        //last selection was not in the scrollable
        //wrap-around: scroll to first row and select first selectable
        create_scrollable_objs(scrollable, 0, 0);
        idx = -1;
    }
    //go to next selectable
    obj = set_selectable_idx(scrollable, idx+1);
    if (obj)
        return obj;
    return (guiObject_t *)scrollable;
}

guiObject_t *GUI_ScrollableGetPrevSelectable(guiScrollable_t *scrollable, guiObject_t *obj)
{
    int idx = -2;
    if (obj)
        //last selection was in the scrollable
        idx = get_selectable_idx(scrollable, obj);

    //first handle any required scrolling, ...selectables can scroll into view
    if ((idx == 0 || idx == -1 || ! scrollable->num_selectable) && scrollable->cur_row > 0)
        //no previous selectable (at first item, or no selectable)
        //and the first row is not visible, just move the scrollbar
        idx = create_scrollable_objs(scrollable, 0, -1);

    else if (idx == -2 && scrollable->cur_row == 0 && scrollable->num_selectable) {
        //last selection was not in the scrollable,
        //wrap-around: scroll to last row and...
        create_scrollable_objs(scrollable, scrollable->item_count, 0);
    }
    if (idx < 0)
        idx = scrollable->num_selectable;   //select last selectable

    //go to previous selectable
    if (idx > 0)
        return set_selectable_idx(scrollable, idx-1);
    return (guiObject_t *)scrollable;
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
