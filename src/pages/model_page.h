#ifndef _MODEL_PAGE_H_
#define _MODEL_PAGE_H_

struct model_page {
    char tmpstr[30];
    char fixed_id[7];
    u8 selected;
    u8 editing;
};

#endif
