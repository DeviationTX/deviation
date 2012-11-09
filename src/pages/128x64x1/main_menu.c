#include "common.h"
#include "pages.h"
//#include "icons.h"
#include "gui/gui.h"
#include "config/model.h"
#include "config/tx.h"

static struct main_menu_page * const mmp = &pagemem.u.main_menu_page;
static u8 action_cb(u32 button, u8 flags, void *data);
static void press_cb(guiObject_t *obj, s8 press_type, const void *data);

static const char *menu_item_name_deviation[] = {
    _tr_noop("Model config"), _tr_noop("Transmitter config"), 0
};

static s8 selected_menu_idx = 0;
/*
 * Main Menu page
 * KEY_UP,KEY_DOWN, KEY_LEFT, KEY_RIGHT: navigate among menu items
 * KEY_ENT: enter sub menu item or function page
 * Key_EXIT: back to the main page
 */
void PAGE_MainMenuInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    PAGE_SetActionCB(action_cb);
    PAGE_ShowHeader(_tr("Main menu"));

    mmp->menu_item_name = (char **)menu_item_name_deviation;
    mmp->main_menu_count = 0;
    while (mmp->menu_item_name[mmp->main_menu_count] != 0) mmp->main_menu_count++;

    int i;
    u8 space = ITEM_HEIGHT + 1;
    u8 row = space;
    u8 col = 1;
    for (i = 0; i < mmp->main_menu_count; i++) {
        mmp->menuItemObj[i] = GUI_CreateLabelBox(col, row, 0, 0,
                &labelDesc, PAGE_menuitem_cb, press_cb, (const void *)mmp->menu_item_name[i]);
        row += ITEM_HEIGHT;
        if (row + ITEM_HEIGHT> LCD_HEIGHT) {
            row = space;
            col = 66;
        }
    }
    //selected_menu_idx = 0;  // to keep main menu selection all the time
    GUI_SetSelected(mmp->menuItemObj[selected_menu_idx]);
}


void PAGE_MainMenuExit()
{
}

static void navigate_items(s8 direction, s8 step)
{
    selected_menu_idx += direction*step;
    if (selected_menu_idx < 0 ) {  // bug fix: push right/left then up/down keys might let selected_menu_idx <0, lead to crash
        selected_menu_idx = mmp->main_menu_count -1;
    }
    if (selected_menu_idx >= mmp->main_menu_count) {
        selected_menu_idx %=  mmp->main_menu_count;
    }
    GUI_SetSelected(mmp->menuItemObj[selected_menu_idx]);
}

u8 action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByName("MainPage", 0);
        } else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            navigate_items(-1, 1);
        }else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
            navigate_items(1, 1);
        } else if (CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
            navigate_items(-1, mmp->main_menu_count > PAGE_ITEM_COUNT? PAGE_ITEM_COUNT :mmp->main_menu_count);
        }  else if (CHAN_ButtonIsPressed(button,BUT_LEFT)) {
            navigate_items(1, mmp->main_menu_count > PAGE_ITEM_COUNT? PAGE_ITEM_COUNT :mmp->main_menu_count);
        } else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

const char *PAGE_menuitem_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    return _tr(data);
}

void press_cb(guiObject_t *obj, s8 press_type, const void *data)
{
    (void)obj;
    (void)press_type;
    const char *str = (const char *)data;
    if (!strcmp("Timer", str)) { // test only
        PAGE_ChangeByName("Mixer", 0);
    }
    else if (!strcmp("Func Conf", str)) {
        PAGE_ChangeByName("SubMenu", 0);
    }
    else if (!strcmp("Model config", str)) {
        PAGE_ChangeByName("SubMenu", 0);
    }
    else if (!strcmp("Transmitter config", str)) {
        PAGE_ChangeByName("SubMenu", 1);
    }
}
