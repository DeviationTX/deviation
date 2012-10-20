#include "common.h"
#include "pages.h"
#include "icons.h"
#include "gui/gui.h"

static struct sub_menu_page * const smp = &pagemem.u.sub_menu_page;
static u8 action_cb(u32 button, u8 flags, void *data);
static const char *idx_string_cb(guiObject_t *obj, const void *data);
static void show_menuItems(u8 startIdx);
static void get_menu_itemName();
static void draw_scrollBar(u8 current_pos, u8 total_count);
static void press_cb(guiObject_t *obj, s8 press_type, const void *data);
static void navigate_item(short direction, u8 step);

#define SUBMENU_COUNT 3
static const char *title[] = {"Function Config", "Model Config", "Tx Config"};  // _tr(
static const char *menu_item_name0[] =  // _tr(
	{"Reverse Switch", "Travel Adjust", "Sub Trim", "D/R & Exp", "Thro hold", "Mix To Throttle",
	  "Tail Curve", "Fail Safe", "Trainer"};
static const char *menu_item_name1[] =  // _tr(
    {"Model Select", "Model Name", "Protocol&Binding", "Type Select", "Swash Type", "Trim Step", "Power Amplifier", "Sensor Setting",
      "Stick Position", "Device Select","Device Output",  "Model Copy", "Model Transmit", "Model Receive"};
static const char *menu_item_name2[] = // _tr(
	{"Language", "Display", "Battery Alarm", "Calibration", "Buzzer", "Vibrator", "Stick Mode", "Stick Direction",
	  "About"};

u8 sub_menu_page = 0;  // global variable to let other page get back to the right sub menu
static u8 selected_menu_idx[SUBMENU_COUNT] = {0, 0, 0};
static u8 current_page_start_idx[SUBMENU_COUNT]  = {0, 0, 0};
static u8 menu_item_count = 0;
static char **menu_item_name;

/*
 * Main Menu page
 * KEY_UP,KEY_DOWN, KEY_LEFT, KEY_RIGHT: navigate among menu items
 * KEY_ENT: enter sub menu item or function page
 * Key_EXIT: back to the main page
 */
void PAGE_SubMenuInit(int page)
{
	sub_menu_page = page;
    PAGE_SetModal(0);
    BUTTON_RegisterCallback(&smp->action,
          CHAN_ButtonMask(BUT_ENTER)
          | CHAN_ButtonMask(BUT_EXIT)
          | CHAN_ButtonMask(BUT_LEFT)
          | CHAN_ButtonMask(BUT_RIGHT)
          | CHAN_ButtonMask(BUT_UP)
          | CHAN_ButtonMask(BUT_DOWN),
          BUTTON_PRESS | BUTTON_LONGPRESS | BUTTON_RELEASE | BUTTON_PRIORITY, action_cb, NULL);

    get_menu_itemName();
	menu_item_count = 0;
	while (menu_item_name[menu_item_count] !=0) menu_item_count++;
    show_menuItems(current_page_start_idx[sub_menu_page]);
    GUI_SetSelected(smp->menuItemObj[selected_menu_idx[sub_menu_page] - current_page_start_idx[sub_menu_page]]);
}

void PAGE_SubMenuExit()
{
	BUTTON_UnregisterCallback(&smp->action);
}

void show_menuItems(u8 startIdx)
{
	GUI_RemoveAllObjects();
	PAGE_ShowHeader(_tr(title[sub_menu_page]));

	int i;
	u8 row = MENU_ITEM_START_ROW;
	u8 col = 1;
	struct LabelDesc labelDesc;
	labelDesc.font = DEFAULT_FONT.font;
	labelDesc.font_color = 0xffff;
	labelDesc.style = LABEL_LEFT;
	labelDesc.outline_color = 1;
	labelDesc.fill_color = labelDesc.outline_color; // not to draw box
	u8 idx_stringWidth = 15;
	u8 idx_stringYOffset = 1;
	current_page_start_idx[sub_menu_page] = startIdx;
	for (i = 0; i < PAGE_ITEM_COUNT; i++) {
		GUI_CreateLabelBox(col, row + idx_stringYOffset, idx_stringWidth, MENU_ITEM_HEIGHT,
			&TINY_FONT, idx_string_cb, NULL, (void *)((long)startIdx +1));
		smp->menuItemObj[i] =GUI_CreateLabelBox(col + idx_stringWidth +2, row, 0, 0,
			&labelDesc, PAGE_menuitem_cb, press_cb, (const void *)menu_item_name[startIdx++]);
		if (startIdx >= menu_item_count) {
			break;
		}
		row += MENU_ITEM_HEIGHT;
	}
	draw_scrollBar(selected_menu_idx[sub_menu_page], menu_item_count);
}

void get_menu_itemName()
{
	switch(sub_menu_page) {
	case 0:
		menu_item_name = (char **)&menu_item_name0[0];
		break;
	case 1:
		menu_item_name = (char **)&menu_item_name1[0];
		break;
	case 2:
		menu_item_name = (char **)&menu_item_name2[0];
		break;
	default:
		return;
	}
}

const char *idx_string_cb(guiObject_t *obj, const void *data)
{
	(void)obj;
	u8 idx = (long)data;
	snprintf(smp->tmpstr, 8, "%d.", idx);
	return smp->tmpstr;
}

void navigate_item(short direction, u8 step)
{
	short expectedIdx = selected_menu_idx[sub_menu_page] + direction* step;
	if (expectedIdx < 0) {
		selected_menu_idx[sub_menu_page] = 0;
	} else if (expectedIdx >= menu_item_count) {
		selected_menu_idx[sub_menu_page] = menu_item_count -1;
	}else {
		selected_menu_idx[sub_menu_page] = expectedIdx;
	}
	if (selected_menu_idx[sub_menu_page] < current_page_start_idx[sub_menu_page] ||
		selected_menu_idx[sub_menu_page] > current_page_start_idx[sub_menu_page] + PAGE_ITEM_COUNT -1) {
		show_menuItems(selected_menu_idx[sub_menu_page]);
	} else {
		GUI_RemoveObj(smp->scrollBar1);
		GUI_RemoveObj(smp->scrollBar2);
		draw_scrollBar(selected_menu_idx[sub_menu_page], menu_item_count);
	}
	GUI_SetSelected(smp->menuItemObj[selected_menu_idx[sub_menu_page] - current_page_start_idx[sub_menu_page]]);

}

u8 action_cb(u32 button, u8 flags, void *data)
{
	(void)data;
	if (flags & BUTTON_PRESS) {
		if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
			PAGE_ChangeByName("MainMenu", 0);
		} else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
			navigate_item(-1, 1);
		}else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
			navigate_item(1, 1);
		} else if (CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
			navigate_item(-1 * PAGE_ITEM_COUNT, 1);
		}  else if (CHAN_ButtonIsPressed(button, BUT_LEFT)) {
			navigate_item(PAGE_ITEM_COUNT, 1);
		} else {
			// only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
			return 0;
		}
	}
	return 1;
}

void draw_scrollBar(u8 current_pos, u8 total_count){
	u8 y = MENU_ITEM_HEIGHT + 1;
	u8 w = 1;
	u8 x = LCD_WIDTH - w - 2;
	u8 h = LCD_HEIGHT - y -3;
	struct LabelDesc labelDesc;
	labelDesc.font = DEFAULT_FONT.font;
	labelDesc.font_color = 0xffff;
	labelDesc.style = LABEL_LEFT;
	labelDesc.outline_color = 1;
	labelDesc.fill_color = labelDesc.outline_color;
	smp->scrollBar1 = GUI_CreateRect(x, y, w, h, &labelDesc);
	u8 box_height = 6;
	y = current_pos * (h-box_height)/(total_count -1)+ y;
	smp->scrollBar2= GUI_CreateRect(x-1, y, w+2, box_height, &labelDesc);
}

/*
 *  invoked when the obj is selected with only ENTER/UP/DOWN is pressed
 */
void press_cb(guiObject_t *obj, s8 press_type, const void *data){
	(void)obj;
	(void)press_type;
	const char *str = (const char *)data;
	if (!strcmp("Calibration", str)) {
		PAGE_ChangeByName("Calibrate", 0);
	}
	else if (!strcmp("Protocol&Binding", str)) {
		PAGE_ChangeByName("ProtoSele", 0);
	}
	else if (!strcmp("Language", str)) {
		PAGE_ChangeByName("SingItem", language);
	}
	else if (!strcmp("Stick Mode", str)) {
		PAGE_ChangeByName("SingItem", stickMode);
	}
	else if (!strcmp("Power Amplifier", str)) {
		PAGE_ChangeByName("SingItem", powerAmplifier);
	}
	else if (!strcmp("Swash Type", str)) {
		PAGE_ChangeByName("SingItem", swashType);
	}
	else if (!strcmp("Model Name", str)) {
		PAGE_ChangeByName("SingItem", modelName);
	}
}
