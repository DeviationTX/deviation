#ifndef _SINGAL_ITEMCONFIG_H_
#define _SINGAL_ITEMCONFIG_H_
#include "buttons.h"
#include "sub_menu.h"

#define MAX_ITEM_COUNT 10
typedef enum  {
    language = 0,
    stickMode,
    swashType,
    battery,
    powerAmplifier,
    modelName,
    mixerMode,
} single_itemConfigType;

struct single_itemCofig_page {
    struct buttonAction action;
    u8 selected;
    int callback_result; // make it int type to align with (void *)
    guiObject_t *titleObj;
    guiObject_t *itemObj;
    guiObject_t *keyboardObj;
    guiObject_t *buttonObj;
    char tmpstr[20];
    char item_content[MAX_ITEM_COUNT][20];
};
#endif
