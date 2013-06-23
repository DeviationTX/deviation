#ifndef _MAIN_LAYOUT_H_
#define _MAIN_LAYOUT_H_

struct layout_page {
    u8 newelem;
    u8 long_press;
    char tmp[20];
    u16 selected_x, selected_y, selected_w, selected_h;
    int selected_for_move;
};

extern int elem_abs_to_rel(int idx);
extern int elem_rel_to_abs(int type, int idx);
extern const char *boxlabel_cb(guiObject_t *obj, const void *data);
extern int MAINPAGE_FindNextElem(unsigned type, int idx);
extern void GetElementSize(unsigned type, u16 *w, u16 *h);
extern void set_selected_for_move(int idx);
extern void show_config();
extern void show_layout();

#endif
