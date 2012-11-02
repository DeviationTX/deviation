#ifndef    _MULTI_ITEMS_CONFIG_H_
#define    _MULTI_ITEMS_CONFIG_H_
#include    "buttons.h"
#include    "sub_menu.h"

typedef    enum        {
                protocol    =    0,
                display,
}    multi_itemConfigType;

#define    MULROW_MAX_ITEM_COUNT    3
struct    multi_items_config_page    {
                struct    buttonAction    action;
                char    label[MULROW_MAX_ITEM_COUNT][15];
                char    item_content[MULROW_MAX_ITEM_COUNT][15];
                guiObject_t    *itemObj[MULROW_MAX_ITEM_COUNT];
                guiObject_t    *buttonObj;
                guiObject_t    *keyboardObj;
                int    callback_result;    //    make    it    int    type    to    align    with    (void    *)
                char    tmpstr[8];
                multi_itemConfigType    config_type;
                u8    item_count;
};
#endif    /*    _MULTI_ITEMS_CONFIG_H_    */
