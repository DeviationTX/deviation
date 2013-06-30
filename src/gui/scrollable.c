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

void create_scrollable_objs(guiScrollable_t *scrollable, int row);
int adjust_row(guiScrollable_t *scrollable, int offset);

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
    scrollable->max_visible_rows = height / row_height + 1;
    if (scrollable->max_visible_rows > item_count)
        scrollable->max_visible_rows = item_count;
    scrollable->visible_rows = scrollable->max_visible_rows;
    adjust_row(scrollable, 0);
    
    GUI_CreateScrollbar(&scrollable->scrollbar,
              x + width - ARROW_WIDTH,
              y,
              height,
              item_count,
              obj,
              scroll_cb, scrollable);
    if (scrollable->max_visible_rows == item_count)
        GUI_SetHidden((guiObject_t *)&scrollable->scrollbar, 1);
    scrollable->cur_row = -1;
    create_scrollable_objs(scrollable, 0);
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

int get_selected_idx(guiScrollable_t *scrollable, guiObject_t *obj)
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
    scrollable->num_selectable = id;
    return found;
}

guiObject_t * set_selected_idx(guiScrollable_t *scrollable, int idx)
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

int adjust_row(guiScrollable_t *scrollable, int offset)
{
    if (! scrollable->size_cb)
        return scrollable->cur_row + offset;
    //This ensures that the next/prev row is visible
    if(offset > 0) {
        int last_row = scrollable->cur_row + scrollable->visible_rows + offset;
        int height = scrollable->max_visible_rows;
        int count = 0;
        while(last_row) {
            int row_height = scrollable->size_cb(last_row-1, scrollable->cb_data);
            if (height - row_height < 0)
                break;
            height -= row_height;
            last_row--;
            count++;
        }
        scrollable->visible_rows = count;
        //printf("Adjusted Next %d -> %d\n", scrollable->cur_row + offset, last_row);
        return last_row;
    } else {
        int height = scrollable->max_visible_rows;
        int first_row = scrollable->cur_row + offset; //We must move at least offset up
        int count = 0;
        int last_row = scrollable->cur_row + scrollable->visible_rows - 1;
        int row = first_row;
        while(row < last_row) {
            int row_height = scrollable->size_cb(row, scrollable->cb_data);
            if (height - row_height < 0)
                break;
            height -= row_height;
            row++;
            count++;
        }
        while(first_row) {
            int row_height = scrollable->size_cb(first_row-1, scrollable->cb_data);
            if (height - row_height < 0)
                break;
            height -= row_height;
            first_row--;
            count++;
        }
        scrollable->visible_rows = count;
        //printf("Adjusted Prev %d -> %d\n", scrollable->cur_row + offset, first_row);
        return first_row;
    }
}

void create_scrollable_objs(guiScrollable_t *scrollable, int row)
{
    if (scrollable->cur_row == row)
        return;
    scrollable->cur_row = row;
    int rel_row = 0;
    int selectable = 0;
    GUI_RemoveScrollableObjs((guiObject_t *)scrollable);
    guiObject_t *head = objHEAD;
    objHEAD = NULL;
    int y = scrollable->header.box.y;
    for(row = scrollable->cur_row; y < scrollable->header.box.y + scrollable->header.box.height; row++, rel_row++) {
        int num_rows = scrollable->size_cb ? scrollable->size_cb(row, scrollable->cb_data) : 1;
        if (rel_row + num_rows > scrollable->max_visible_rows)
            break;
        selectable += scrollable->row_cb(row, rel_row, y, scrollable->cb_data);
        y += scrollable->row_height * num_rows;
    }
    scrollable->visible_rows = rel_row;
    scrollable->num_selectable = selectable;
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
}

guiObject_t *select_scrollable(guiScrollable_t *scrollable, int row, int col)
{
    if (scrollable->getobj_cb) {
        while(1) {
            guiObject_t *obj = scrollable->getobj_cb(row, col, NULL);
            if (! obj)
                return NULL;
            if (! OBJ_IS_HIDDEN(obj))
                return obj;
            col = col + (col >= 0 ? 1 : -1);
        }
    }
    return NULL;
}

#if HAS_TOUCH
int scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data) {
    (void)parent;
    (void)pos;
    guiScrollable_t *scrollable = (guiScrollable_t *)data;
    int adjust;
    if (direction == 2) {
        if (scrollable->cur_row + 2 * scrollable->visible_rows > scrollable->item_count)
            adjust = scrollable->item_count - (scrollable->cur_row + scrollable->visible_rows);
        else
            adjust = scrollable->visible_rows;
    } else if (direction == -2) {
        if (scrollable->cur_row - scrollable->visible_rows < 0)
            adjust = -scrollable->cur_row;
        else
            adjust = -scrollable->visible_rows;
    } else if (direction == 1) {
        if (scrollable->cur_row + scrollable->visible_rows == scrollable->item_count)
            adjust = 0;
        else
            adjust = 1;
    } else {
        if (scrollable->cur_row == 0)
            adjust = 0;
        else
            adjust = -1;
    }
    if (adjust)
        create_scrollable_objs(scrollable, adjust_row(scrollable, adjust));
    return -1;
}
#endif

guiObject_t *GUI_ScrollableGetNextSelectable(guiScrollable_t *scrollable, guiObject_t *obj)
{
    if(obj) {
        //last selection was in the scrollable
        int idx = get_selected_idx(scrollable, obj);
        if(idx == -1) {
            //selection is not in Scrollable
            return NULL;
        }
        if(idx == scrollable->num_selectable -1) {
            //At the last visible item
            if (scrollable->cur_row + scrollable->visible_rows == scrollable->item_count) {
                //At the last item in the last row
                //Go to the next item after the Scrollable
                return (guiObject_t *)scrollable;
            }
            //The number of visible rows may change, so we must calculate the
            //selection based on absolute position
            int next_row = scrollable->cur_row + scrollable->visible_rows;
            create_scrollable_objs(scrollable, adjust_row(scrollable, 1));
            return select_scrollable(scrollable, next_row - scrollable->cur_row, 0);
        }
        obj = obj->next;
    } else {
        //last selection was not in the scrollable
        if (! scrollable->num_selectable) {
            //No selectable objects, just move the scrollbar
            if (scrollable->cur_row + scrollable->visible_rows < scrollable->item_count) {
                create_scrollable_objs(scrollable, adjust_row(scrollable, 1));
            }
            return (guiObject_t *)scrollable;
        }
        obj = scrollable->head;
        if (scrollable->cur_row) {
            create_scrollable_objs(scrollable, 0);
        }
    }
    while(obj && ! OBJ_IS_SELECTABLE(obj)) {
        obj = obj->next;
    }
    return obj;
}

guiObject_t *GUI_ScrollableGetPrevSelectable(guiScrollable_t *scrollable, guiObject_t *obj)
{
    if(obj) {
        //last selection was in the scrollable
        int idx = get_selected_idx(scrollable, obj);
        if(idx == -1) {
            //selection is not in Scrollable
            return NULL;
        }
        if(idx == 0) {
            //At the first visible item
            if (scrollable->cur_row == 0) {
                //At the first item in the first row
                //Go to the prev item before the Scrollable
                return (guiObject_t *)scrollable;
            }
            int new_row = adjust_row(scrollable, -1);
            int adjust = scrollable->cur_row - new_row - 1;
            create_scrollable_objs(scrollable, new_row);
            return select_scrollable(scrollable, adjust, -1);
        }
        return set_selected_idx(scrollable, idx-1);
    } else {
        //last selection was not in the scrollable
        if (! scrollable->num_selectable) {
            //No selectable objects, just move the scrollbar
            if (scrollable->cur_row)
                create_scrollable_objs(scrollable, adjust_row(scrollable, -1));
            return (guiObject_t *)scrollable;
        }
        int adjust = scrollable->item_count - (scrollable->cur_row + scrollable->visible_rows);
        create_scrollable_objs(scrollable, adjust_row(scrollable, adjust));
        return select_scrollable(scrollable, scrollable->visible_rows - 1, -1);
    }
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
        int row = absrow;
        if (row + scrollable->visible_rows >= scrollable->item_count)
            row = scrollable->item_count - scrollable->visible_rows;
        create_scrollable_objs(scrollable, row);
    }
    int relrow = absrow - scrollable->cur_row;
    return scrollable->getobj_cb(relrow, col, scrollable->cb_data);
}

guiObject_t *GUI_ShowScrollableRowOffset(guiScrollable_t *scrollable, int row_idx)
{
    create_scrollable_objs(scrollable, row_idx >> 8);
    return set_selected_idx(scrollable, row_idx & 0xff);
}
int GUI_ScrollableGetObjRowOffset(guiScrollable_t *scrollable, guiObject_t *obj)
{
    return (scrollable->cur_row << 8) | get_selected_idx(scrollable, obj);
}
