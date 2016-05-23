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

#include "_scrollable.c"
#if HAS_TOUCH
    static int scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data);
#else
    #define scroll_cb NULL
#endif

static void create_scrollable_objs(guiScrollable_t *scrollable, int row);
static int has_selectable(guiScrollable_t *scrollable);

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
    create_scrollable_objs(scrollable, 0);
    //force selection to be current object-if there are no selectable contents
    if (! has_selectable(scrollable))
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

void GUI_RemoveScrollableObjs(struct guiObject *obj)
{
    struct guiScrollable *scrollable = (struct guiScrollable *)obj;
    guiObject_t *head = objHEAD;
    objHEAD = scrollable->head;
    GUI_RemoveAllObjects();
    objHEAD = head;
    scrollable->head = NULL;
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


static guiObject_t *get_selectable_obj(guiScrollable_t *scrollable, int row, int col)
{
    guiObject_t *head = scrollable->head;
    guiObject_t *last_head = NULL;
    int cur_row = scrollable->cur_row-1;
    int cur_col = 0;
    while(head) {
        if (OBJ_IS_ROWSTART(head)) {
            if (row == cur_row && col < 0) {
                return last_head;
            }
            cur_row++;
            cur_col = 0;
        }
        if(! OBJ_IS_HIDDEN(head) && OBJ_IS_SELECTABLE(head)) {
            if(row == cur_row) {
                if (col == cur_col) {
                    return head;
                }
                last_head = head;
                cur_col++;
            }
        }
        head = head->next;
    }
    if (row == cur_row && col < 0) {
        return last_head;
    }
    return NULL;
}

static guiObject_t * get_last_object()
{
    guiObject_t *head = objHEAD;
    if (! head)
        return NULL;
    while(head->next)
        head = head->next;
    return head;
}

static int get_obj_abs_row_col(guiScrollable_t *scrollable, guiObject_t *obj, int *row, int *col)
{
    guiObject_t *head = scrollable->head;
    *row = scrollable->cur_row-1;
    *col = 0;
    while(head) {
        if (OBJ_IS_ROWSTART(head)) {
            (*row)++;
            *col = 0;
        }
        if(! OBJ_IS_HIDDEN(head) && OBJ_IS_SELECTABLE(head)) {
            if(head == obj)
                return 1;
            (*col)++;
        }
        head = head->next;
    }
    return 0;
}

static guiObject_t *set_selected_abs_row_col(guiScrollable_t *scrollable, int row, int col)
{
    //Note that col must be >= -1 and row must be between >=0 and <=item_count
    //1st we normalize row and column, including wrap-around
    if (col < 0)
        row--;
    if (row < 0)
        return NULL; //We shouldn't wrap here.  Let the main GUI code do the wrap for us
        //row = scrollable->item_count-1;
    if (row >= scrollable->item_count) {
        return NULL;
        //row = 0;
        //col = 0;
    }
    if (row >= scrollable->cur_row && row < scrollable->cur_row + scrollable->visible_rows) {
        //requested row is already visible
        guiObject_t *obj = get_selectable_obj(scrollable, row, col);
        if (obj)
            return obj;
        row++;
        col = 0;
        if (row >= scrollable->item_count) {
            return NULL;
        }
    }
    create_scrollable_objs(scrollable, row);
    return get_selectable_obj(scrollable, row, col);
}


static guiObject_t * set_selectable_idx(guiScrollable_t *scrollable, int idx)
{
    int num_rows = 0;
    guiObject_t *head = scrollable->head;
    guiObject_t *last_head = NULL;
    int id = 0;
    if (idx < 0)
        return (guiObject_t *)scrollable;
    while(head) {
        if (OBJ_IS_ROWSTART(head)) {
            num_rows++;
            if(num_rows > scrollable->selectable_rows)
                break; //exclude not completely visible items on last row
        }
        if(! OBJ_IS_HIDDEN(head) && OBJ_IS_SELECTABLE(head)) {
            if(id == idx)
                return head;
            id++;
            last_head = head;
        }
        head = head->next;
    }
    return last_head;
}

static int has_selectable(guiScrollable_t *scrollable) {
    guiObject_t *head = scrollable->head;
    while(head) {
        if (! OBJ_IS_HIDDEN(head) && OBJ_IS_SELECTABLE(head)) {
            return 1;
        }
        head = head->next;
    }
    return 0;
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


//This ensures that the requested row is completely visible
//and that first_row is kept within a valid range
//it will also do the minimal change possible from the current position
static int adjust_row(guiScrollable_t *scrollable, int target_row)
{
    int maxrow = scrollable->item_count - 1;
    int target = (target_row < 0) ? 0 : (target_row > maxrow) ? maxrow : target_row;
    int first_row = scrollable->cur_row;
    int last_row = scrollable->cur_row;
    int max_y = scrollable->header.box.height;

    if (target <= first_row)
        return target;
    while (1) {
        int y = 0;
        last_row = 0;
        for(int row = first_row; row <= maxrow; row++) {
            y += scrollable->row_height * (scrollable->size_cb ?
                                       scrollable->size_cb(row, scrollable->cb_data) : 1);
            if (y > max_y)
                break;
            last_row = row;
        }
        if (target <= last_row)
            break;
        first_row++;
    }
    return first_row;
}

static void create_scrollable_objs(guiScrollable_t *scrollable, int row)
{
    if (row >= 0) {
        row = adjust_row(scrollable, row);
    } else {
        //Force row #
        row = - row;
    }
    int offset = row - scrollable->cur_row;
    if (scrollable->head && offset == 0)
        return;
    scrollable->cur_row = row;
    int rel_row = 0;
    int selectable_rows = 0;
    GUI_RemoveScrollableObjs((guiObject_t *)scrollable);

    guiObject_t *head = objHEAD;
    objHEAD = NULL;
    for(int y = scrollable->header.box.y, bottom = y + scrollable->header.box.height;
        y < bottom && row < scrollable->item_count && rel_row < scrollable->max_visible_rows;
        row++, rel_row++)
    {
        guiObject_t *row_start = get_last_object();
        scrollable->row_cb(row, rel_row, y, scrollable->cb_data);
        OBJ_SET_ROWSTART(row_start ? row_start->next : objHEAD, 1);
        y += scrollable->row_height * (scrollable->size_cb ? 
                                       scrollable->size_cb(row, scrollable->cb_data) : 1);
        if (y > bottom + scrollable->row_height - ROW_HEIGHT_OFFSET)
            break;  //is not selectable because it's not completely visible
        selectable_rows++;
    }
    scrollable->visible_rows = rel_row;
    scrollable->selectable_rows = selectable_rows;
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
    int hidden = (rel_row >= scrollable->item_count) ? 1 : 0;
    GUI_SetHidden((guiObject_t *)&scrollable->scrollbar, hidden);
    if (! hidden)
        GUI_SetScrollbar(&scrollable->scrollbar, scroll_pos);
}

#if HAS_TOUCH
static int scroll_cb(guiObject_t *parent, u8 pos, s8 direction, void *data) {
    (void)parent;
    (void)pos;
    int sel_row = -1;
    int sel_col = 0;
    guiScrollable_t *scrollable = (guiScrollable_t *)data;
    int row = scrollable->cur_row;
    if (direction > 1)
        row += scrollable->visible_rows;
    else if (direction < -1)
        row -= scrollable->visible_rows;
    else
        row += direction;
    if (row < 0) {
        row = 0;
    } else if (row > scrollable->item_count - scrollable->visible_rows) {
        row = scrollable->item_count - scrollable->visible_rows;
    }
    guiObject_t *sel = GUI_GetSelected();
    if (sel) {
        if(get_obj_abs_row_col(scrollable, sel, &sel_row, &sel_col)) {
            if (sel_row < row) {
                sel_row = row;
                sel_col = 0;
            }
        } else {
            sel_row = -1;
        }
    }
    create_scrollable_objs(scrollable, -row);
    if (sel_row >= 0) {
        if(sel_row >= row + scrollable->visible_rows) {
            sel_row = row + scrollable->visible_rows - 1;
            sel_col = 0;
        }
        GUI_SetSelected(set_selected_abs_row_col(scrollable, sel_row, sel_col));
    }
    return -1;
}
#endif

guiObject_t *GUI_ScrollableGetNextSelectable(guiScrollable_t *scrollable, guiObject_t *obj)
{
    int row, col;
    if (! obj) {
        if (! has_selectable(scrollable)) {
            //there are no selectable rows, increment by one if possible, no wrap-around
            row = scrollable->cur_row + scrollable->visible_rows;
            if (row >= scrollable->item_count)
                row = scrollable->item_count-1;
            create_scrollable_objs(scrollable, row);
            return NULL;
        }
        row = 0;
        col = 0;
    } else {
        get_obj_abs_row_col(scrollable, obj, &row, &col);
        col++;
    }
    return set_selected_abs_row_col(scrollable, row, col);
}

guiObject_t *GUI_ScrollableGetPrevSelectable(guiScrollable_t *scrollable, guiObject_t *obj)
{
    int row, col;
    if (! obj) {
        if (! has_selectable(scrollable)) {
            //there are no selectable rows, increment by one if possible, no wrap-around
            row = scrollable->cur_row - 1;
            if (row < 0)
                row = 0;
            create_scrollable_objs(scrollable, row);
            return NULL;
        }
        row = scrollable->item_count;
        col = -1;
    } else {
        get_obj_abs_row_col(scrollable, obj, &row, &col);
        col--;
    }
    return set_selected_abs_row_col(scrollable, row, col);
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
    return set_selected_abs_row_col(scrollable, absrow, col);
}

guiObject_t *GUI_ShowScrollableRowOffset(guiScrollable_t *scrollable, int row_idx)
{
    //printf("Restoring position:%04x\n", row_idx);
    create_scrollable_objs(scrollable, -(row_idx >> 8));

    //number of rows and/or visible rows can change, ie. added, deleted or model changed
    int idx = row_idx & 0xff;
    return set_selectable_idx(scrollable, idx);
}
int GUI_ScrollableGetObjRowOffset(guiScrollable_t *scrollable, guiObject_t *obj)
{
    int idx = get_selectable_idx(scrollable, obj);
    if (idx < 0)
        idx = 0;
    idx |= (scrollable->cur_row << 8);
    //printf("Saving position:%04x\n", idx);
    return idx;
}
int GUI_ScrollableCurrentRow(guiScrollable_t *scrollable)
{
    return scrollable->cur_row;
}
int GUI_ScrollableVisibleRows(guiScrollable_t *scrollable)
{
    return scrollable->visible_rows;
}
