
extern int GetWidgetLoc(void *ptr, u16 *x, u16 *y, u16 *w, u16 *h);
extern int MAINPAGE_FindNextElem(unsigned type, int idx);
extern void GetElementSize(unsigned type, u16 *w, u16 *h);

static const char *label_cb(guiObject_t *obj, const void *data);
static void touch_cb(guiObject_t *obj, s8 press, const void *data);
static void set_selected_for_move(guiLabel_t * obj);

static void show_config();
static void select_for_move(guiLabel_t *obj);
static void notify_cb(guiObject_t *obj);

guiLabel_t *selected_for_move;
u16 selected_x, selected_y, selected_w, selected_h;
char tmp[20];
static u8 long_press;
static u8 newelem;

int elem_abs_to_rel(int idx)
{
    unsigned type = ELEM_TYPE(pc.elem[idx]);
    int nxt = -1;
    for (int i = 0; i < NUM_ELEMS-1; i++) {
        nxt = MAINPAGE_FindNextElem(type, nxt+1);
        if (nxt == idx)
            return i;
    }
    return 0;
}

int elem_rel_to_abs(int type, int idx)
{
    int nxt = -1;
    for(int i = 0; i < idx+1; i++)
        nxt = MAINPAGE_FindNextElem(type, nxt+1);
    return nxt;
}

int elem_get_count(int type)
{
    int nxt = -1;
    for (int i = 0; i < NUM_ELEMS; i++) {
        nxt = MAINPAGE_FindNextElem(type, nxt+1);
        if (nxt == -1)
            return i;
    }
    return 0;
}

void draw_elements()
{
    u16 x, y, w, h;
    int i;
    set_selected_for_move(NULL);
    guiObject_t *obj = gui->y.header.next;
    if (obj)
        GUI_RemoveHierObjects(obj);
    for (i = 0; i < NUM_ELEMS; i++) {
        if (! GetWidgetLoc(&pc.elem[i], &x, &y, &w, &h))
            break;
        int type = ELEM_TYPE(pc.elem[i]);
        const char *(*strCallback)(guiObject_t *, const void *) = label_cb;
        void *data = (void *)(long)elem_abs_to_rel(i);
        int desc = 0;
        switch(type) {
            case ELEM_MODELICO:
                desc = 0; strCallback = NULL; data = (void *)_tr("Model");
                break;
            case ELEM_HTRIM:
            case ELEM_VTRIM:
                desc = 1;
                break;
            case ELEM_SMALLBOX:
            case ELEM_BIGBOX:
                desc = 2;
#ifndef NUMERIC_LABELS
                strCallback = boxlabel_cb;
#endif
                data = (void *)(long)i;
                break;
            case ELEM_BAR:
                desc = 3;
                break;
            case ELEM_TOGGLE:
                desc = 4;
        }
        GUI_CreateLabelBox(&gui->elem[i], x, y, w, h, &gui->desc[desc], strCallback, touch_cb, data);
    }
}

const char *label_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int idx = (long)data;
    sprintf(tmp, "%d", idx+1);
    return tmp;
}
const char *boxlabel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int i = (long)data;
    if (pc.elem[i].src) {
        if (pc.elem[i].src <= NUM_TIMERS)
            return TIMER_Name(tmp, pc.elem[i].src - 1);
        else if( pc.elem[i].src - NUM_TIMERS <= NUM_TELEM)
        return TELEMETRY_Name(tmp, pc.elem[i].src - NUM_TIMERS);
    }
    return INPUT_SourceName(tmp, pc.elem[i].src
               ? pc.elem[i].src - (NUM_TELEM + NUM_TIMERS) + NUM_INPUTS
               : 0);
}

void touch_cb(guiObject_t *obj, s8 press, const void *data)
{
    //press = -1 : release
    //press = 0  : short press
    //press = 1  : long press
    (void)data;
    if (long_press) {
        if(press == -1)
            long_press = 0;
        return;
    }
    if(press < 0) {
        select_for_move((guiLabel_t *)obj);
    }
    if(selected_for_move && press == 1) {
        show_config();
        long_press = 1;
    }
}

int guielem_idx(guiObject_t *obj)
{
    return ((unsigned long)obj - (unsigned long)gui->elem) / sizeof(guiLabel_t);
}

const char *newelem_cb(guiObject_t *obj, int dir, void *data)
{   
    (void)data;
    (void)obj;
    newelem = GUI_TextSelectHelper(newelem, 0, ELEM_LAST-1, dir, 1, 1, NULL);
    switch(newelem) {
        case ELEM_SMALLBOX: return _tr("Small-box");
        case ELEM_BIGBOX:   return _tr("Big-box");
        case ELEM_TOGGLE:   return _tr("Toggle");
        case ELEM_BAR:      return _tr("Bargraph");
        case ELEM_VTRIM:    return _tr("V-trim");
        case ELEM_HTRIM:    return _tr("H-trim");
        case ELEM_MODELICO: return _tr("Model");
    }
    return "";
}

int create_element()
{
    int i;
    u16 x,y,w,h;
    for (i = 0; i < NUM_ELEMS; i++)
        if (! ELEM_USED(pc.elem[i]))
            break;
    if (i == NUM_ELEMS)
        return -1;
    y = 1;
    GetElementSize(newelem, &w, &h);
    x = (LCD_WIDTH - w) / 2;
    y = (((LCD_HEIGHT - HEADER_Y) - h) / 2) + HEADER_Y;
    memset(&pc.elem[i], 0, sizeof(struct elem));
    ELEM_SET_X(pc.elem[i], x);
    ELEM_SET_Y(pc.elem[i], y);
    ELEM_SET_TYPE(pc.elem[i], newelem);
    return i;
}

static const char *add_dlgbut_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    return data ? _tr("Add") : _tr("Load");
}

void move_elem()
{
    guiObject_t *obj = GUI_GetSelected();
    if ((guiLabel_t *)obj < gui->elem)
        return;
    int idx = guielem_idx(obj);
    ELEM_SET_X(pc.elem[idx], selected_x);
    ELEM_SET_Y(pc.elem[idx], selected_y);
    draw_elements();
    select_for_move((guiLabel_t *)obj);
}

void notify_cb(guiObject_t *obj)
{
    if ((guiLabel_t *)obj < gui->elem)
        return;
    int idx = guielem_idx(obj);
    selected_x = ELEM_X(pc.elem[idx]);
    selected_y = ELEM_Y(pc.elem[idx]);
    GetElementSize(ELEM_TYPE(pc.elem[idx]), &selected_w, &selected_h);
    GUI_Redraw((guiObject_t *)&gui->x);
    GUI_Redraw((guiObject_t *)&gui->y);

}

static const char *dlgts_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    int idx = (long)data;
    int type = ELEM_TYPE(pc.elem[idx]);
    switch (type) {
        case ELEM_SMALLBOX:
        case ELEM_BIGBOX:
        {
            pc.elem[idx].src = GUI_TextSelectHelper(pc.elem[idx].src, 0, NUM_TELEM + NUM_TIMERS + NUM_CHANNELS, dir, 1, 1, NULL);   
            return boxlabel_cb(NULL, data);
        }
        case ELEM_BAR:
            pc.elem[idx].src = GUI_TextSelectHelper(pc.elem[idx].src, 0, NUM_CHANNELS, dir, 1, 1, NULL);   
            return INPUT_SourceName(tmp, pc.elem[idx].src ? pc.elem[idx].src + NUM_INPUTS : 0);
        case ELEM_TOGGLE:
        {
            int val = MIXER_SRC(pc.elem[idx].src);
            int newval = GUI_TextSelectHelper(val, 0, NUM_SOURCES, dir, 1, 1, NULL);
            newval = INPUT_GetAbbrevSource(val, newval, dir);
            if (val != newval) {
                val = newval;
                pc.elem[idx].src = val;
            }
            return INPUT_SourceNameAbbrevSwitch(tmp, pc.elem[idx].src);
        }
        case ELEM_HTRIM:
        case ELEM_VTRIM:
            pc.elem[idx].src = GUI_TextSelectHelper(pc.elem[idx].src, 0, NUM_TRIMS, dir, 1, 1, NULL);
            sprintf(tmp, "%s%d", _tr("Trim"),pc.elem[idx].src + 1);
            return tmp;
    }
    return "";
}

static void dlgbut_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    int idx = (long)data;
    int i;
    //Remove object
    int type = ELEM_TYPE(pc.elem[idx]);
    for(i = idx+1; i < NUM_ELEMS; i++) {
        if (! ELEM_USED(pc.elem[i]))
            break;
        pc.elem[i-1] = pc.elem[i];
    }
         ELEM_SET_Y(pc.elem[i-1], 0);
    idx = MAINPAGE_FindNextElem(type, 0);
    if (idx >= 0) {
        selected_for_move = &gui->elem[idx];
    } else {
        selected_for_move = NULL;
    }
    //close the dialog and reopen with new elements
    show_config();
}

static void add_dlgbut_cb(struct guiObject *obj, const void *data)
{
    (void)obj;
    if(data) {
    } else {
        PAGE_MainLayoutExit();
        MODELPage_ShowLoadSave(LOAD_LAYOUT, PAGE_MainLayoutInit);
    }
}

