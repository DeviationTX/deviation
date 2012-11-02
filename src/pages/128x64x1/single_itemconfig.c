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
#include "music.h"
#include <stdlib.h>

static struct single_itemCofig_page * const sicp = &pagemem.u.single_itemCofig_page;
const char *title[] = {  // _tr(  following items need to put in lang files
    "Language",
    "Stick Mode",
    "Swash Type",
    "Battery Alarm",
    "Power Amplifier",
    "Fixed Id",
    "Model Name",
};

static u8 action_cb(u32 button, u8 flags, void *data);
static void press_cb_item(guiObject_t *obj, s8 press_type, const void *data);
static void refresh_itemName();
static const char *value_cb(guiObject_t *obj, const void *data);
static const char *value_cb_title(guiObject_t *obj, const void *data);

static u8 item_count = 0;
static single_itemConfigType config_type = language;
/*
 * Main Menu page
 * KEY_UP,KEY_DOWN, KEY_LEFT, KEY_RIGHT: navigate among menu items
 * KEY_ENT: enter sub menu item or function pagel
 * Key_EXIT: back to the main page
 */
void PAGE_SingleItemConfigInit(int page)
{
    config_type = page;
    PAGE_SetModal(0);
    PAGE_SetActionCB(action_cb);

    sicp->titleObj = GUI_CreateLabelBox(1, 1, MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT,
                    &DEFAULT_FONT, value_cb_title, NULL,  NULL);
    GUI_SetSelectable(sicp->titleObj, 0);

    sicp->buttonObj = NULL;
    refresh_itemName();

    struct LabelDesc labelDesc;
    labelDesc.font = DEFAULT_FONT.font;
    labelDesc.font_color = 0xffff;
    labelDesc.style = LABEL_CENTER;  // set selected will invert the color aumatically
    labelDesc.outline_color = 1;
    labelDesc.fill_color = labelDesc.outline_color; // not to draw box

    u8 width = 80;
    u8 row = MENU_ITEM_START_ROW + 2;
    u8 col = (LCD_WIDTH - width)/2;
    sicp->itemObj = GUI_CreateLabelBox(col, row, width, MENU_ITEM_HEIGHT,
                &labelDesc, value_cb, press_cb_item ,  NULL);
    GUI_SetSelectable(sicp->itemObj, 1);
    GUI_SetSelected(sicp->itemObj);
}


void PAGE_SingleItemConfigExit()
{
}

const char *value_cb(guiObject_t *obj, const void *data){
    (void)obj;
    (void)data;
    return (const char *)sicp->item_content[sicp->selected];
}

const char *value_cb_title(guiObject_t *obj, const void *data){
    (void)obj;
    (void)data;
    strcpy((char *)sicp->tmpstr, (const char *)_tr(title[config_type]));
    strcat((char *)sicp->tmpstr, ":");
    return sicp->tmpstr;
}

static void keyboard_done_cb(guiObject_t *obj, void *data)
{
    (void)obj;
    (void)data;
    GUI_RemoveObj(sicp->keyboardObj);
    if (sicp->callback_result == 1) {
        if (config_type == modelName) { // show keyboard
            strncpy(Model.name, (const char *)sicp->tmpstr, sizeof(Model.name));
        }
        //Save model info here so it shows up on the model page
        CONFIG_SaveModelIfNeeded();
        MUSIC_Play(MUSIC_SAVING);
        refresh_itemName();
    }
}

static void show_keyboard(enum KeyboardType keyboardType, char *text, s32 max_size) {
    sicp->keyboardObj = GUI_CreateKeyboard(keyboardType, text, max_size, keyboard_done_cb, (void *)&sicp->callback_result);
}

static void navigate_options(short direction)
{
    // navigate among options in a object
    strncpy(sicp->tmpstr, (const char *)sicp->item_content[sicp->selected], sizeof(sicp->tmpstr)); // don't let the keyboard change target directly as we might press Ext to cancel the change
    if (config_type == modelName) { // show keyboard
        show_keyboard(KEYBOARD_ALPHA, sicp->tmpstr, 10); // no more than 10 chars is allowed for model name
    } else {
        short expectedIdx = sicp->selected + direction;
        if (expectedIdx < 0) {
            sicp->selected = 0;//item_count -1;  // do we want rewind ?
        } else if (expectedIdx >= item_count) {
            sicp->selected = item_count -1 ;//0;
            if (sicp->selected >= 254)  sicp->selected = 0;
        }else {
            sicp->selected = expectedIdx;
        }
        GUI_Redraw(sicp->itemObj);
    }
}

u8 action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    if ((flags & BUTTON_PRESS) || (flags & BUTTON_LONGPRESS)) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            PAGE_ChangeByName("SubMenu", sub_menu_item); // sub_menu_page is defined in sub_menu.c
        } else if (CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
            navigate_options(-1);
        }  else if (CHAN_ButtonIsPressed(button, BUT_LEFT)) {
            navigate_options(1);
        }  else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            guiObject_t *obj = GUI_GetSelected();
            GUI_SetSelected((guiObject_t *)GUI_GetPrevSelectable(obj));
        }  else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
            guiObject_t *obj = GUI_GetSelected();
            GUI_SetSelected((guiObject_t *)GUI_GetNextSelectable(obj));
        }
        else {
            return 0; // to let press call back to handle the ENT key
        }
    }
    return 1;
}

void refresh_itemName() {
    sicp->selected = 0;
    item_count = 1;
    switch (config_type) {
    case language:
        strcpy((char *)sicp->item_content[0], (const char *)"English");
        item_count = 1;
        if (FS_OpenDir("language")) {
            FILE *fh;
            char filename[13];
            int type;
            while((type = FS_ReadDir(filename)) != 0) {
                if (type == 1 && strncasecmp(filename, "lang", 4) == 0) {
                    sprintf(sicp->tmpstr, "language/%s", filename);
                    fh = fopen(sicp->tmpstr, "r");
                    if (fh) {
                        if(fgets(sicp->tmpstr, sizeof(sicp->tmpstr), fh) == NULL)
                            sicp->tmpstr[0] = 0;
                        fclose(fh);
                        unsigned len = strlen(sicp->tmpstr);
                        if(strlen(sicp->tmpstr) && sicp->tmpstr[0] != ':') {
                            sicp->tmpstr[len-1] = '\0';
                            strcpy((char *)sicp->item_content[item_count], (const char *)sicp->tmpstr);
                        }
                    }
                    item_count++;
                    if (item_count >= MAX_ITEM_COUNT) break;
                }
            }
            FS_CloseDir();
        }
        sicp->selected = Transmitter.language;
        break;
    case stickMode:
        item_count = 4;
        for (u8 i = 0; i < item_count; i++) {
            sprintf((char *)sicp->item_content[i], _tr("Mode %d"), i+1);
        }
        sicp->selected = Transmitter.mode -1; // MODE_1=1,
        break;
    case powerAmplifier:
        item_count = TXPOWER_LAST;
        for (u8 i = 0; i < item_count; i++) {
            sprintf((char *)sicp->item_content[i], RADIO_TX_POWER_VAL[i]);
        }
        sicp->selected = Model.tx_power;
        break;
    case swashType:
        item_count = SWASH_TYPE_LAST;
        strcpy((char *)sicp->item_content[0], "1 Servo");
        for (u8 i = 0; i < item_count; i++) {
            switch(i) {
                case SWASH_TYPE_NONE:
                    strcpy((char *)sicp->item_content[i], "1 Servo"); //return _tr("None");
                    break;
                case SWASH_TYPE_120:
                    strcpy((char *)sicp->item_content[i], "120");
                    break;
                case SWASH_TYPE_120X:
                    strcpy((char *)sicp->item_content[i], "120x");
                    break;
                case SWASH_TYPE_140:
                    strcpy((char *)sicp->item_content[i], "140");
                    break;
                case SWASH_TYPE_90:
                    strcpy((char *)sicp->item_content[i], "90");
                    break;
            }
        }
        sicp->selected = Model.swash_type;
        break;
    case modelName:
        strncpy((char *)sicp->item_content[0], (const char *)Model.name, sizeof(sicp->item_content[0]));
        break;
    default:
        break;
    }
}

/**
 * save the change after ENT is pressed
 */
void press_cb_item(guiObject_t *obj, s8 press_type, const void *data)
{
    (void)obj;
    (void)press_type;
    (void)data;
    switch (config_type) {
    case language:
        CONFIG_ReadLang(sicp->selected);
        GUI_Redraw(sicp->titleObj);
        MUSIC_Play(MUSIC_SAVING);
        break;
    case stickMode:
        Transmitter.mode = sicp->selected + 1;
        MUSIC_Play(MUSIC_SAVING);
        break;
    case powerAmplifier:
        Model.tx_power = sicp->selected;
        MUSIC_Play(MUSIC_SAVING);
        break;
    case swashType:
        Model.swash_type = sicp->selected;
        MUSIC_Play(MUSIC_SAVING);
        break;
    case modelName:
        //strncpy(Model.name, (const char *)&sicp->item_name[0], sizeof(Model.name)); // it is saved when pressing Done via the keyboard
        break;
    default:
        break;
    }
    // Todo: Generate a sound for saving
}
