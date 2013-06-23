#ifndef _MAIN_LAYOUT_H_
#define _MAIN_LAYOUT_H_

extern int elem_abs_to_rel(int idx);
extern int elem_rel_to_abs(int type, int idx);
extern const char *boxlabel_cb(guiObject_t *obj, const void *data);
extern int MAINPAGE_FindNextElem(unsigned type, int idx);
extern void GetElementSize(unsigned type, u16 *w, u16 *h);
extern void set_selected_for_move(int idx);
extern void show_config();
extern void show_layout();

extern guiLabel_t *selected_for_move;

#endif
