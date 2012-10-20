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
#include "pages.h"
#include "gui/gui.h"
#include "config/tx.h"
#include "config/model.h"
#include <stdlib.h>

static struct protocol_select_page * const psp = &pagemem.u.protocol_select_page;

static u8 action_cb(u32 button, u8 flags, void *data);
static const char *value_cb(guiObject_t *obj, const void *data);
static const char *value_cb_chanNum(guiObject_t *obj, const void *data);
static const char *value_cb_fixedId(guiObject_t *obj, const void *data);
static void bind();

void PAGE_ProtocolSelectInit(int page)
{
	(void)page;
    PAGE_SetModal(0);
    BUTTON_RegisterCallback(&psp->action,
          CHAN_ButtonMask(BUT_ENTER)
          | CHAN_ButtonMask(BUT_EXIT)
          | CHAN_ButtonMask(BUT_LEFT)
          | CHAN_ButtonMask(BUT_RIGHT)
          | CHAN_ButtonMask(BUT_UP)
          | CHAN_ButtonMask(BUT_DOWN),
          BUTTON_PRESS | BUTTON_LONGPRESS | BUTTON_RELEASE | BUTTON_PRIORITY, action_cb, NULL);

    u8 y = 1;
    u8 x = 0;
    GUI_CreateLabelBox(x, y, 0, 0, &DEFAULT_FONT, NULL, NULL,  (const void *)_tr("Protocol:"));
    y += MENU_ITEM_HEIGHT;
    GUI_CreateLabelBox(x, y, 0, 0, &DEFAULT_FONT, NULL, NULL,  (const void *)_tr("Channel#:"));
    y += MENU_ITEM_HEIGHT;
	GUI_CreateLabelBox(x, y, 0, 0, &DEFAULT_FONT, NULL, NULL,  (const void *)_tr("Fixed Id:"));
	struct LabelDesc labelDesc;
	labelDesc.font = DEFAULT_FONT.font;
	labelDesc.font_color = 0xffff;
	labelDesc.style = LABEL_CENTER;  // set selected will invert the color aumatically
	labelDesc.outline_color = 1;
	labelDesc.fill_color = labelDesc.outline_color; // not to draw box
    u8 width = 50;
    x += 70;
    y = 1 ;
	psp->protocolObj = GUI_CreateLabelBox(x, y, width, MENU_ITEM_HEIGHT,
			&labelDesc, value_cb, NULL ,  NULL);
	GUI_SetSelectable(psp->protocolObj, 1);

	y += MENU_ITEM_HEIGHT;
	psp->numChannelObj = GUI_CreateLabelBox(x, y, width, MENU_ITEM_HEIGHT,
			&labelDesc, value_cb_chanNum, NULL ,  NULL);
	GUI_SetSelectable(psp->numChannelObj, 1);

	y += MENU_ITEM_HEIGHT;
	psp->fixedIdObj = GUI_CreateLabelBox(x, y, width, MENU_ITEM_HEIGHT,
			&labelDesc, value_cb_fixedId, NULL ,  NULL);
	GUI_SetSelectable(psp->fixedIdObj, 1);

	y += MENU_ITEM_HEIGHT + 2;
	psp->buttonObj = GUI_CreateButton((LCD_WIDTH - 32)/2, y, BUTTON_DEVO10, NULL, 0x0000,
			NULL, (void *)_tr("Bind"));
	GUI_SetSelectable(psp->buttonObj, 1);

	GUI_SetSelected(psp->protocolObj);
}


void PAGE_ProtocolSelectExit()
{
	BUTTON_UnregisterCallback(&psp->action);
}

const char *value_cb(guiObject_t *obj, const void *data){
	(void)obj;
	(void)data;
	return ProtocolNames[Model.protocol];
}

const char *value_cb_chanNum(guiObject_t *obj, const void *data){
	(void)obj;
	(void)data;
    sprintf(psp->tmpstr, "%d", Model.num_channels);
    return psp->tmpstr;
}

const char *value_cb_fixedId(guiObject_t *obj, const void *data) {
	(void)obj;
	(void)data;
	if(Model.fixed_id == 0)
		strncpy((char *)psp->tmpstr, _tr("None"), sizeof(psp->tmpstr));
	else
		sprintf((char *)psp->tmpstr, "%d", (int)Model.fixed_id);
    return psp->tmpstr;
}

static void keyboard_done_cb(guiObject_t *obj, void *data)
{
	(void)obj;
	(void)data;
	GUI_RemoveObj(psp->keyboardObj);
	if (psp->callback_result == 1) {
		Model.fixed_id = atoi((char *)psp->tmpstr);
	}
}

static void navigate_item(short direction)
{
	if (GUI_GetSelected() == psp->fixedIdObj) {
		if(Model.fixed_id == 0)
			strncpy((char *)psp->tmpstr, "0", sizeof(psp->tmpstr));
		else
			sprintf((char *)psp->tmpstr, "%d", (int)Model.fixed_id);
		psp->keyboardObj = GUI_CreateKeyboard(KEYBOARD_NUM, psp->tmpstr, 999999,
				keyboard_done_cb, (void *)&psp->callback_result);
	}
	// navigate options of a object
	u8 select = 0;
	guiObject_t *obj = GUI_GetSelected();
	if (obj == psp->protocolObj) {
		select = Model.protocol + direction;
		if (select == 0) select = PROTOCOL_COUNT -1; // rewind
		else if (select >= PROTOCOL_COUNT) select = 1;
		Model.protocol = select;
		Model.num_channels= PROTOCOL_NumChannels();
		GUI_Redraw(psp->numChannelObj);
		GUI_Redraw(psp->protocolObj);
	} else if (obj == psp->numChannelObj) {
		u8 max = PROTOCOL_NumChannels();
		select = Model.num_channels + direction;
		if (select == 0) select = max; // rewind
		else if (select > max) select = 1;
		Model.num_channels = select;
		GUI_Redraw(psp->numChannelObj);
	}
}

u8 action_cb(u32 button, u8 flags, void *data)
{
	(void)data;
	if (flags & BUTTON_PRESS) {
		if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
			PAGE_ChangeByName("SubMenu", sub_menu_page); // sub_menu_page is defined in sub_menu.c
		} else if (CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
			navigate_item(-1);
		}  else if (CHAN_ButtonIsPressed(button, BUT_LEFT)) {
			navigate_item(1);
		}  else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
			guiObject_t *obj = GUI_GetSelected();
			GUI_SetSelected((guiObject_t *)GUI_GetPrevSelectable(obj));
		}  else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
			guiObject_t *obj = GUI_GetSelected();
			GUI_SetSelected((guiObject_t *)GUI_GetNextSelectable(obj));
		} else if (CHAN_ButtonIsPressed(button, BUT_ENTER)){
			guiObject_t *obj = GUI_GetSelected();
			if (obj == psp->buttonObj) {
				bind();
			} else {
				GUI_SetSelected((guiObject_t *)GUI_GetNextSelectable(obj));
			}
		} else {
			return 0; // to let press call back to handle the ENT key
		}
	}
	return 1;
}

void bind()
{
	if (PROTOCOL_AutoBindEnabled())
		PROTOCOL_Init(0);
	else
		PROTOCOL_Bind();  // bind command is done  bind() of devo.c or initialize of dsm2.c
}
