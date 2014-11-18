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
    scrollable->max_visible_rows = (height + 4) / row_height; // prevent paint of not fully displayable items
    if (scrollable->max_visible_rows > item_count)
        scrollable->max_visible_rows = item_count;
    
    GUI_CreateScrollbar(&scrollable->scrollbar,
              x + width - ARROW_WIDTH,
              y,
              height,
              item_count,
              obj,
              scroll_cb, scrollable);
    if (scrollable->max_visible_rows == item_count)
        GUI_SetHidden((guiObject_t *)&scrollable->scrollbar, 1);
    create_scrollable_objs(scrollable, 0, 0);
    //force selection to be current objectif there are no selectable contents
    if (scrollable->num_selectable == 0)
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
            if(id == idx)
                return head;
            id++;
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
    int maxrow = scrollable->item_count - height;
    if (row < 0) row = 0;
    if (! scrollable->size_cb)
        return row > maxrow ? maxrow : row;

    maxrow = scrollable->item_count - 1;
    if (row > maxrow)
        row = maxrow;

    //This ensures that the next/prev row is completely visible
    int first_row = row, count = 0; //int orig_row = row;
    while(row <= maxrow && height) {
        height -= scrollable->size_cb(row, scrollable->cb_data);
        if (height < 0)
            break;
        row++;
        count++;
    }
    if (height < 0) {
        if (count && offset < 0)
            count--;
        //Restart and work backwards to make all of last row visible
        height = scrollable->max_visible_rows;
        first_row += count + 1;
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
    int idx = get_selectable_idx(scrollable, objSELECTED);  //Save index of selectable
    int old_rel_last_row = scrollable->visible_rows - 1;

    //adjust row and calculate visible_rows
    if (offset) {
        row = adjust_row(scrollable, scrollable->cur_row + offset, offset);
    } else {
        row = adjust_row(scrollable, row, row - scrollable->cur_row);
    }
    old_rel_last_row -= offset = row - scrollable->cur_row;
    if (scrollable->head && offset == 0)
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
            break;  //Is not really selectable because it is not completely visible

        //Maintain index of saved selectable
        if (idx != -1 && offset < 0) {
            idx += selectable; offset++; }
        if (offset > 0 && rel_row > old_rel_last_row)
            idx -= selectable;

        num_selectable += selectable;
    }
    if (offset > 0)
        idx += num_selectable - scrollable->num_selectable;
    scrollable->cur_row = row - rel_row;
    scrollable->visible_rows = rel_row;
    scrollable->num_selectable = num_selectable;
    scrollable->head = objHEAD;
    objHEAD = head;
    head = scrollable->head;
    while(head) {
        OBJ_SET_SCROLLABLE(head, 1);
        head = head->next;
    }
    int scroll_pos = (2 * scrollable->cur_row + rel_row) / 2;
    if (scrollable->cur_row == 0)
        scroll_pos = 0;
    else if(scrollable->cur_row + rel_row == scrollable->item_count)
        scroll_pos = scrollable->item_count - 1;
    if (! OBJ_IS_HIDDEN((guiObject_t *)&scrollable->scrollbar))
        GUI_SetScrollbar(&scrollable->scrollbar, scroll_pos);

    //Return index of saved selectable
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
    int adjust;
    if (direction > 1)
        adjust = scrollable->visible_rows;
    else if (direction < -1)
        adjust = -scrollable->visible_rows;
    else
        adjust = direction;

    int row = scrollable->cur_row + adjust;
    create_scrollable_objs(scrollable, row, 0);
    return -1;
}
#endif

guiObject_t *GUI_ScrollableGetNextSelectable(guiScrollable_t *scrollable, guiObject_t *obj)
{
    int i = 0;
    if(obj) {
        //last selection was in the scrollable
        int idx = get_selectable_idx(scrollable, obj);
        if(idx == scrollable->num_selectable -1) {
            //At the last visible item
            if (scrollable->cur_row + scrollable->visible_rows == scrollable->item_count) {
                //At the last item in the last row
                //Go to the next item after the Scrollable
                return (guiObject_t *)scrollable;
            }
            idx = create_scrollable_objs(scrollable, 0, 1);
        }
        //Go to next selectable
        obj = set_selectable_idx(scrollable, idx+1);
        if(obj)
            return obj;
        i = 1;
    }
    //No next selectable objects, just move the scrollbar
    if (scrollable->cur_row + scrollable->visible_rows < scrollable->item_count) {
        if (! scrollable->num_selectable) i = 1;
        int idx = create_scrollable_objs(scrollable, 0, i); //Selectables can scroll into view
        obj = set_selectable_idx(scrollable, idx+1);        //Go to next selectable
        if(obj)
            return obj;
        return (guiObject_t *)scrollable;
    }
    //last selection was not in the scrollable
    //scroll to first row and select first selectable
    create_scrollable_objs(scrollable, 0, 0);
    return set_selectable_idx(scrollable, 0);
}

guiObject_t *GUI_ScrollableGetPrevSelectable(guiScrollable_t *scrollable, guiObject_t *obj)
{
    int i = 0;
    if(obj) {
        int idx = get_selectable_idx(scrollable, obj);
        if(idx == 0) {
            //At the first visible item
            if (scrollable->cur_row == 0) {
                //At the first item in the first row
                //Go to the prev item before the Scrollable
                return (guiObject_t *)scrollable;
            }
            idx = create_scrollable_objs(scrollable, 0, -1);
        }
        //Go to previous selectable
        if (idx > 0)
            return set_selectable_idx(scrollable, idx-1);
        i = -1;
    }
    //No prev selectable objects, just move the scrollbar
    if (scrollable->cur_row > 0) {
        if (! scrollable->num_selectable) i = -1;
        int idx = create_scrollable_objs(scrollable, scrollable->cur_row, i);
        if (idx == -1) idx = scrollable->num_selectable;
        if (idx > 0)                                        //Selectables scrolled into view
            return set_selectable_idx(scrollable, idx-1);   //Go to prev selectable
        return (guiObject_t *)scrollable;
    }
    //last selection was not in the scrollable,
    //scroll to last row and select last selectable
    create_scrollable_objs(scrollable, scrollable->item_count -1, 0);
    return set_selectable_idx(scrollable, scrollable->num_selectable -1);
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
