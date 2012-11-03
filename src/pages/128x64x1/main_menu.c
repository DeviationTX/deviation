#include "common.h"
#include "pages.h"
#include "icons.h"
#include "gui/gui.h"
#include "config/model.h"
#include "config/tx.h"

static struct main_menu_page * const mmp = &pagemem.u.main_menu_page;
static u8 action_cb(u32 button, u8 flags, void *data);
static void press_cb(guiObject_t *obj, s8 press_type, const void *data);

static const char *menu_item_name_deviation[] = {
    "Model Conf", "Tx Conf", 0
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
    PAGE_ShowHeader(_tr("Main Menu"));

    mmp->menu_item_name = (char **)menu_item_name_deviation;
    mmp->main_menu_count = 0;
    while (mmp->menu_item_name[mmp->main_menu_count] != 0) mmp->main_menu_count++;

    int i;
    u8 row = MENU_ITEM_START_ROW;
    u8 col = 1;
    struct LabelDesc labelDesc;
    labelDesc.font = DEFAULT_FONT.font;
    labelDesc.font_color = 0xffff;
    labelDesc.style = LABEL_LEFT;
    labelDesc.outline_color = 1;
    labelDesc.fill_color = labelDesc.outline_color; // not to draw box
    for (i = 0; i < mmp->main_menu_count; i++) {
        mmp->menuItemObj[i] = GUI_CreateLabelBox(col, row, 0, 0,
                &labelDesc, PAGE_menuitem_cb, press_cb, (const void *)mmp->menu_item_name[i]);
        row += MENU_ITEM_HEIGHT;
        if (row + MENU_ITEM_HEIGHT> LCD_HEIGHT) {
            row = MENU_ITEM_START_ROW;
            col = 66;
        }
    }
    //selected_menu_idx = 0;  // to keep main menu selection all the time
    GUI_SetSelected(mmp->menuItemObj[selected_menu_idx]);
}


void PAGE_MainMenuExit()
{
}

u8 action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByName("MainPage", 0);
        } else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            selected_menu_idx--;
            if (selected_menu_idx < 0 ) {
                selected_menu_idx = mmp->main_menu_count -1;
            }
            GUI_SetSelected(mmp->menuItemObj[selected_menu_idx]);
        }else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
            selected_menu_idx++;
            if (selected_menu_idx >= mmp->main_menu_count) {
                selected_menu_idx = 0;
            }
            GUI_SetSelected(mmp->menuItemObj[selected_menu_idx]);
        } else if (CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
            selected_menu_idx -= PAGE_ITEM_COUNT;
            if (selected_menu_idx < 0) {
                selected_menu_idx = mmp->main_menu_count + selected_menu_idx;
            }
            GUI_SetSelected(mmp->menuItemObj[selected_menu_idx]);
        }  else if (CHAN_ButtonIsPressed(button,BUT_LEFT)) {
            selected_menu_idx += PAGE_ITEM_COUNT;
            if (selected_menu_idx >= mmp->main_menu_count) {
                selected_menu_idx %=  mmp->main_menu_count;
            }
            GUI_SetSelected(mmp->menuItemObj[selected_menu_idx]);
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
    else if (!strcmp("Model Conf", str)) {
        PAGE_ChangeByName("SubMenu", 0);
    }
    else if (!strcmp("Tx Conf", str)) {
        PAGE_ChangeByName("SubMenu", 1);
    }
}
