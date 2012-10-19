#ifndef _MODEL_PAGE_H_
#define _MODEL_PAGE_H_

enum {
   LOAD_MODEL,
   SAVE_MODEL,
   LOAD_TEMPLATE,
   LOAD_ICON,
};

struct model_page {
    char tmpstr[30];
    char iconstr[24];
    char fixed_id[7];
    u8 file_state;
    guiObject_t *chanObj;
/*Load save */
    guiObject_t *obj;
    u8 selected;
    void(*return_page)(int page);
    enum ModelType modeltype;
};

#endif
