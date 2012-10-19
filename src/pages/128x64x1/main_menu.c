#include "common.h"
#include "pages.h"
#include "icons.h"
#include "gui/gui.h"
#include "config/model.h"
#include "config/tx.h"

static struct main_menu_page * const mmp = &pagemem.u.main_menu_page;
static u8 action_cb(u32 button, u8 flags, void *data);
static void press_cb(guiObject_t *obj, s8 press_type, const void *data);

static char *menu_item_name[] = {
	"Func Conf", "Model Conf", "Tx Conf", "Monitor", "Thro Curve", "Pit Curve" //, "Prog Mix", "Timer"
};
static short selected_menu_idx = 0;
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
    BUTTON_RegisterCallback(&mmp->action,
          CHAN_ButtonMask(BUT_ENTER)
          | CHAN_ButtonMask(BUT_EXIT)
          | CHAN_ButtonMask(BUT_LEFT)
          | CHAN_ButtonMask(BUT_RIGHT)
          | CHAN_ButtonMask(BUT_UP)
          | CHAN_ButtonMask(BUT_DOWN),
          BUTTON_PRESS | BUTTON_LONGPRESS | BUTTON_RELEASE | BUTTON_PRIORITY, action_cb, NULL);

    PAGE_ShowHeader(_tr("Main Menu"));

    int i;
    u8 row = MENU_ITEM_START_ROW;
    u8 col = 1;
    struct LabelDesc labelDesc;
	labelDesc.font = DEFAULT_FONT.font;
	labelDesc.font_color = 0xffff;
	labelDesc.style = LEFT;
	labelDesc.outline_color = 1;
	labelDesc.fill_color = labelDesc.outline_color; // to not draw box
    for (i = 0; i < MAIN_MENU_ITEM_COUNT; i++) {
    	mmp->menuItemObj[i] = GUI_CreateLabelBox(col, row, MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT,
    			&labelDesc, PAGE_menuitem_cb, press_cb, (const void *)menu_item_name[i]);
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
	BUTTON_UnregisterCallback(&mmp->action);
}

u8 action_cb(u32 button, u8 flags, void *data)
{
	(void)data;
	if (flags & BUTTON_PRESS) {
		if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
			PAGE_ChangeByName("MainPage", 0);
		} else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
			selected_menu_idx--;
			if (selected_menu_idx < 0 ) {
				selected_menu_idx = MAIN_MENU_ITEM_COUNT -1;
			}
			GUI_SetSelected(mmp->menuItemObj[selected_menu_idx]);
		}else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
			selected_menu_idx++;
			if (selected_menu_idx >= MAIN_MENU_ITEM_COUNT) {
				selected_menu_idx = 0;
			}
			GUI_SetSelected(mmp->menuItemObj[selected_menu_idx]);
		} else if (CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
			selected_menu_idx -= PAGE_ITEM_COUNT;
			if (selected_menu_idx < 0) {
				selected_menu_idx = MAIN_MENU_ITEM_COUNT + selected_menu_idx;
			}
			GUI_SetSelected(mmp->menuItemObj[selected_menu_idx]);
		}  else if (CHAN_ButtonIsPressed(button,BUT_LEFT)) {
			selected_menu_idx += PAGE_ITEM_COUNT;
			if (selected_menu_idx >= MAIN_MENU_ITEM_COUNT) {
				selected_menu_idx %=  MAIN_MENU_ITEM_COUNT;
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
	if (!strcmp(menu_item_name[3], str)) {
		PAGE_ChangeByName("Monitor", 0);
	}
	else if (!strcmp(menu_item_name[4], str)) { // test only
		PAGE_ChangeByName("Calibrate", 0);
	}
	else if (!strcmp(menu_item_name[0], str)) {
		PAGE_ChangeByName("SubMenu", 0);
	}
	else if (!strcmp(menu_item_name[1], str)) {
		PAGE_ChangeByName("SubMenu", 1);
	}
	else if (!strcmp(menu_item_name[2], str)) {
		PAGE_ChangeByName("SubMenu", 2);
	}
}
