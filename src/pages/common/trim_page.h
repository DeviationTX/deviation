#ifndef __TRIM_PAGE_H_
#define __TRIM_PAGE_H_

struct trim_page {
    u8 modifying_trim;
    u8 index;
    struct Trim trim;
    char tmpstr[30];
};

#endif
