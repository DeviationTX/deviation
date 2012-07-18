#ifndef _MODEL_PAGE_H_
#define _MODEL_PAGE_H_

struct model_page {
    char tmpstr[30];
    char iconstr[20];
    char fixed_id[7];
    guiObject_t *icon;
    u8 selected;
    u8 editing;
    enum ModelType modeltype;
};

#endif
