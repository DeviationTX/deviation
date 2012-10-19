#ifndef _PROTOCOL_SELECT_H_
#define _PROTOCOL_SELECT_H_
#include "buttons.h"
#include "sub_menu.h"

struct protocol_select_page {
    struct buttonAction action;
    guiObject_t *protocolObj;
    guiObject_t *numChannelObj;
    guiObject_t *fixedIdObj;
    guiObject_t *buttonObj;
    guiObject_t *keyboardObj;
    int callback_result; // make it int type to align with (void *)
    char tmpstr[8];
};
#endif
