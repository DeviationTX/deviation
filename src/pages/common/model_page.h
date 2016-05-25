#ifndef __MODEL_PAGE_H_
#define __MODEL_PAGE_H_

enum loadSaveType {
   LOAD_MODEL,
   SAVE_MODEL,
   LOAD_TEMPLATE,
   LOAD_ICON,
   LOAD_LAYOUT,
};

struct model_page {
    char iconstr[24];
    char fixed_id[7];
    u8 file_state;
    u8 last_mixermode;
    u8 last_txpower;
/*Load save */
    u8 total_items;
    u8 menu_type;
    enum ModelType modeltype;
};

#endif
