#ifndef _MODEL_PAGE_H_
#define _MODEL_PAGE_H_

struct model_page {
    char tmpstr[30];
    char iconstr[20];
    char fixed_id[7];
/*Load save */
    guiObject_t *icon;
    u8 selected;
    void(*return_page)(int page);
    enum ModelType modeltype;
};

#endif
