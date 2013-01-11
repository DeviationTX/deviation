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

//static u8 press_cb(u32 button, u8 flags, void *data);
void create_scrollable_objs(guiScrollable_t *scrollable, int row);

guiObject_t *GUI_CreateScrollable(guiScrollable_t *scrollable, u16 x, u16 y, u16 width, u16 height, u8 row_height, u8 item_count,
     int (*row_cb)(int absrow, int relrow, int x, void *data),
     guiObject_t * (*getobj_cb)(int relrow, int col, void *data),
     void *data)
{
    struct guiObject *obj = (guiObject_t *)scrollable;
    struct guiBox    *box;

    box = &obj->box;
    scrollable->row_cb = row_cb;
    scrollable->getobj_cb = getobj_cb;
    scrollable->row_height = row_height;
    scrollable->item_count = item_count;
    scrollable->cb_data = data;
    scrollable->head = NULL;
    scrollable->cur_row = -1;

    box->x = x;
    box->y = y;
    box->width = width;
    box->height = height;

    obj->Type = Scrollable;
    OBJ_SET_TRANSPARENT(obj, 0);
    OBJ_SET_SELECTABLE(obj, 1); //Scrollables aren't really selectable
    connect_object(obj);
    scrollable->visible_rows = height / row_height + 1;

    GUI_CreateScrollbar(&scrollable->scrollbar,
              x + width - ARROW_WIDTH,
              y,
              height,
              item_count - scrollable->visible_rows + 1,
              obj,
              NULL, NULL);

    create_scrollable_objs(scrollable, 0);
    //force selection to be current objectif there are no selectable contents
    if (scrollable->num_selectable == 0)
        objSELECTED = obj;

    //BUTTON_RegisterCallback(&scrollable->action,
    //     CHAN_ButtonMask(BUT_UP)
    //     | CHAN_ButtonMask(BUT_DOWN)
    //     BUTTON_PRESS, press_cb, obj);
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

void GUI_RemoveScrollableObjs(struct guiObject *obj)
{
    struct guiScrollable *scrollable = (struct guiScrollable *)obj;
    guiObject_t *head = objHEAD;
    objHEAD = scrollable->head;
    GUI_RemoveAllObjects();
    objHEAD = head;
}

int get_selected_idx(guiScrollable_t *scrollable, guiObject_t *obj)
{
    guiObject_t *head = scrollable->head;
    if (! obj)
        return -1;
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
    for(row = scrollable->cur_row; rel_row < scrollable->visible_rows; row++, rel_row++) {
        selectable += scrollable->row_cb(row, rel_row, scrollable->header.box.y + rel_row * scrollable->row_height, scrollable->cb_data);
    }
    scrollable->num_selectable = selectable;
    scrollable->head = objHEAD;
    objHEAD = head;
    head = scrollable->head;
    while(head) {
        OBJ_SET_SCROLLABLE(head, 1);
        head = head->next;
    }
    GUI_SetScrollbar(&scrollable->scrollbar, scrollable->cur_row);
    
}

guiObject_t *select_scrollable(guiScrollable_t *scrollable, int row, int col)
{
    return scrollable->getobj_cb ? scrollable->getobj_cb(row, col, NULL) : NULL;
}
guiObject_t *GUI_ScrollableGetNextSelectable(guiScrollable_t *scrollable, guiObject_t *obj)
{
    if(obj) {
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
            create_scrollable_objs(scrollable, scrollable->cur_row+1);
            return select_scrollable(scrollable, scrollable->visible_rows-1, 0);
        }
        obj = obj->next;
    } else {
        if (! scrollable->num_selectable) {
            //No selectable objects, just move the scrollbar
            if (scrollable->cur_row + scrollable->visible_rows < scrollable->item_count) {
                create_scrollable_objs(scrollable, scrollable->cur_row+1);
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
            create_scrollable_objs(scrollable, scrollable->cur_row -1);
            return select_scrollable(scrollable, 0, -1);
        }
        return set_selected_idx(scrollable, idx-1);
    } else {
        if (! scrollable->num_selectable) {
            //No selectable objects, just move the scrollbar
            if (scrollable->cur_row)
                create_scrollable_objs(scrollable, scrollable->cur_row -1);
            return (guiObject_t *)scrollable;
        }
        create_scrollable_objs(scrollable, scrollable->item_count - scrollable->visible_rows);
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
