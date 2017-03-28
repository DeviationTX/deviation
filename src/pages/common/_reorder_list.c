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
static struct reorder_obj * const gui = &gui_objs.u.reorder;
struct {
    void(*return_page)(u8 *);
    const char *(*text_cb)(u8 idx);
    u8 *list;
    u8 count;
    u8 selected;
    u8 max;
    u8 copyto;
} rl;
enum {
    MOVE_UP,
    MOVE_DOWN,
    APPLY,
    INSERT,
    REMOVE,
    SAVE,
};

static void redraw()
{
    GUI_Redraw(&gui->value);
    GUI_Redraw(&gui->copy);
    GUI_Redraw(&gui->scrollable);
    for(unsigned i = 0; i < sizeof(gui->name) / sizeof(guiLabel_t); i++) {
        GUI_Redraw(&gui->name[i]);
    }
}

static void display_list(int idx)
{
    GUI_ShowScrollableRowCol(&gui->scrollable, idx, 0);
}

static void okcancel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    if (data) {
        rl.return_page(rl.list);
        MUSIC_Play(MUSIC_SAVING);
    }
    rl.return_page(NULL);
    PAGE_Pop();
}

void press_button_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 tmp;
    switch((long)data) {
    case MOVE_UP:
        if(rl.selected > 0) {
            tmp = rl.list[rl.selected-1];
            rl.list[rl.selected-1] = rl.list[rl.selected];
            rl.list[rl.selected] = tmp;
            rl.selected--;
            redraw();
            //display_list(rl.selected -1);
            //GUI_ListBoxSelect(&gui->list, rl.selected - 1);
        }
        break;
    case MOVE_DOWN:
        if(rl.selected < rl.count - 1) {
            tmp = rl.list[rl.selected+1];
            rl.list[rl.selected+1] = rl.list[rl.selected];
            rl.list[rl.selected] = tmp;
            rl.selected++;
            redraw();
            //display_list(rl.selected +1);
            //GUI_ListBoxSelect(&gui->list, rl.selected + 1);
        }
        break;
    case APPLY:
        if(rl.selected != rl.copyto)
            rl.list[rl.copyto] = rl.list[rl.selected];
        redraw();
        //display_list(rl.selected);
        //GUI_Redraw(&gui->list);
        break;
    case INSERT:
        if(rl.count < rl.max) {
            for(tmp = rl.count; tmp > rl.selected; tmp--)
                rl.list[tmp] = rl.list[tmp-1];
            rl.count++;
            rl.list[rl.selected] = 255;
            redraw();
            //display_list(rl.selected+1);
            //GUI_ListBoxSelect(&gui->list, rl.selected + 1);
        }
        break;
    case REMOVE:
        if(rl.count > 1) {
            for(tmp = rl.selected; tmp < rl.count; tmp++)
                rl.list[tmp] = rl.list[tmp+1];
            rl.list[rl.count] = 0;
            rl.count--;
            if(rl.selected == rl.count)
                rl.selected--;
            redraw();
            //display_list(rl.selected);
            //GUI_ListBoxSelect(&gui->list, rl.selected);
        }
        break;
    }
    return;
}

const char *copy_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    rl.copyto = GUI_TextSelectHelper(rl.copyto, 0, rl.count - 1, dir, 1, 5, NULL);
    return(rl.text_cb(rl.list[rl.copyto]));
}

const char *value_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    rl.selected = GUI_TextSelectHelper(rl.selected, 0, rl.count - 1, dir, 1, 5, NULL);
    display_list(rl.selected);
    return(rl.text_cb(rl.list[rl.selected]));
}

static const char *list_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int idx = (long)data;
    return rl.text_cb(rl.list[idx]);
}

void PAGE_ShowReorderList(u8 *list, u8 count, u8 selected, u8 max_allowed, const char *(*text_cb)(u8 idx), void(*return_page)(u8 *))
{
    rl.return_page = return_page;
    rl.list = list;
    rl.selected = selected;
    rl.copyto = selected;
    rl.count = count;
    rl.text_cb = text_cb;
    rl.max = max_allowed;
   PAGE_PushByID(PAGEID_REORDER, 0);
}
