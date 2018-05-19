/*
 This project is ffree software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Deviation is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Deviation.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "common.h"
#define ENABLE_GUIOBJECT
#include "gui.h"
#include "target_defs.h"
#include "config/display.h"
#include "_mapped_gfx.h"

struct guiObject *objHEAD     = NULL;
struct guiObject *objTOUCHED  = NULL;
struct guiObject *objSELECTED = NULL;
struct guiObject *objModalButton = NULL;
struct guiObject *objDIALOG   = NULL;
static void (*select_notify)(guiObject_t *obj) = NULL;

static struct guiObject *objACTIVE = NULL;

static buttonAction_t button_action;
static buttonAction_t button_modalaction;
u8 FullRedraw;

#if HAS_TOUCH
static u8 change_selection_on_touch = 1;
static u8 in_touch = 0;
#endif

static unsigned handle_buttons(u32 button, unsigned flags, void*data);
#include "_gui.c"
 
void connect_object(struct guiObject *obj)
{
    if (objHEAD == NULL) {
        objHEAD = obj;
    } else {
        struct guiObject *ptr = objHEAD;
        while(ptr->next)
            ptr = ptr->next;
        ptr->next = obj;
    }
    OBJ_SET_USED(obj, 1);
    OBJ_SET_DIRTY(obj, 1);
    obj->next = NULL;
}

u8 coords_in_box(struct guiBox *box, struct touch *coords)
{
    unsigned result = ((coords->x == box->x || (coords->x > box->x && coords->x < box->x + box->width))
                    && (coords->y == box->y || (coords->y > box->y && coords->y < box->y + box->height)));
    //printf("(%dx%d)-(%dx%d) <-> (%dx%d) : %d\n", box->x, box->y, box->x + box->width, box->y + box->height, coords->x, coords->y, result);
    return result;
}

void GUI_DrawObject(struct guiObject *obj)
{
#ifdef DEBUG_DRAW
    switch (obj->Type) {
    case UnknownGUI: printf("Draw Unknown: "); break;
    case CheckBox:   printf("Draw Checkbox:"); break;
    case Dropdown:   printf("Draw Dropdown:"); break;
    case Button:     printf("Draw Button:  "); break;
    case Label:      printf("Draw Label:   "); break;
    case Image:      printf("Draw Image:   "); break;
    case Dialog:     printf("Draw Dialog:  "); break;
    case XYGraph:    printf("Draw XYGraph: "); break;
    case BarGraph:   printf("Draw BarGraph:"); break;
    case TextSelect: printf("Draw TextSel: "); break;
    case Keyboard:   printf("Draw Keyboard:"); break;
    case Scrollbar:  printf("Draw ScrlBar: "); break;
    case Scrollable: printf("Draw Scrlable:"); break;
    case Rect:       printf("Draw Rect:    "); break;
    }
    printf(" ptr: %08x Selected: %s\n", obj, obj == objSELECTED ? "true" : "false");
#endif
    switch (obj->Type) {
    case UnknownGUI: break;
    case CheckBox:   break;
    case Dropdown:   break;
    case Button:     GUI_DrawButton(obj);            break;
    case Label:      GUI_DrawLabel(obj);             break;
    case Image:      GUI_DrawImage(obj);             break;
    case Dialog:     GUI_DrawDialog(obj);            break;
    case XYGraph:    GUI_DrawXYGraph(obj);           break;
    case BarGraph:   GUI_DrawBarGraph(obj);          break;
    case TextSelect: GUI_DrawTextSelect(obj);        break;
    case Keyboard:   GUI_DrawKeyboard(obj);          break;
    case Scrollbar:  GUI_DrawScrollbar(obj);         break;
    case Scrollable: GUI_DrawScrollable(obj);        break;
    case Rect:       GUI_DrawRect(obj);              break;
    }
    if (obj == objSELECTED && obj->Type != Scrollable)
        _gui_hilite_selected(obj);
    OBJ_SET_DIRTY(obj, 0);
}

void GUI_DrawObjects(void)
{
    struct guiObject *obj = objHEAD;
    while(obj) {
        if(! OBJ_IS_HIDDEN(obj)) {
            GUI_DrawObject(obj);
        } else {
            OBJ_SET_DIRTY(obj, 0);
        }
        obj = obj->next;
    }
}

void GUI_HandleInput(int source, int value) {
    if(objSELECTED && objSELECTED->Type == TextSelect) {
        guiTextSelect_t *select = (guiTextSelect_t *)objSELECTED;
        if (select->InputValueCB) {
            select->InputValueCB(objSELECTED, source, value, select->cb_data);
            OBJ_SET_DIRTY(objSELECTED, 1);
        }
    }
}

void GUI_RemoveAllObjects()
{
    while(objHEAD)
        GUI_RemoveObj(objHEAD);
    FullRedraw = REDRAW_EVERYTHING;
}

void GUI_RemoveObj(struct guiObject *obj)
{
    switch(obj->Type) {
    case Dialog: {
        GUI_HandleModalButtons(0);
        GUI_RemoveHierObjects(obj->next);
        objDIALOG = NULL;
        break;
    }
    case Scrollbar:
        BUTTON_UnregisterCallback(&((guiScrollbar_t *)obj)->action);
        break;
    case Scrollable:
        GUI_RemoveScrollableObjs(obj);
        GUI_RemoveObj((guiObject_t *)&((guiScrollable_t *)obj)->scrollbar);
        break;
    case Keyboard:
        BUTTON_UnregisterCallback(&((guiKeyboard_t *)obj)->action);
        break;
    case Image:
    case XYGraph:
        _GUI_UnmapWindow(1);
        break;
    default: break;
    }
    
    if (objTOUCHED == obj)
        objTOUCHED = NULL;
    if (objSELECTED == obj)
        objSELECTED = NULL;
    OBJ_SET_USED(obj, 0);
    // Reattach linked list
    struct guiObject *prev = objHEAD;
    if (prev == obj) {
        objHEAD = obj->next;
    } else {
        while(prev) {
            if(prev->next == obj) {
                prev->next = obj->next;
                break;
            }
            prev = prev->next;
        }
    }
    if(obj->Type == Dialog) {
        FullRedraw = REDRAW_ONLY_DIRTY;
    } else {
        FullRedraw = objHEAD ? REDRAW_IF_NOT_MODAL : REDRAW_EVERYTHING;
    }
}

void GUI_RemoveHierObjects(struct guiObject *obj)
{
    if(obj == objHEAD) {
        GUI_RemoveAllObjects();
        return;
    }
    struct guiObject *parent = objHEAD;
    while(parent && parent->next != obj)
        parent = parent->next;
    if(! parent)
        return;
    while(parent->next)
        GUI_RemoveObj(parent->next);
    FullRedraw = REDRAW_IF_NOT_MODAL;
}

void GUI_SetHidden(struct guiObject *obj, u8 state)
{
    if((!!state) == (!! OBJ_IS_HIDDEN(obj)))
        return;
    OBJ_SET_HIDDEN(obj, state);
    OBJ_SET_DIRTY(obj, 1);
}

void GUI_DrawBackground(u16 x, u16 y, u16 w, u16 h)
{
    if(w == 0 || h == 0)
        return;
    if(FullRedraw != REDRAW_ONLY_DIRTY) {
        if(w == LCD_WIDTH && h == LCD_HEIGHT)
            _gui_draw_background(x, y, w, h); //Optimization to prevent partial redraw when it isn't needed
        return;  //Optimization to prevent partial redraw when it isn't needed
    }
    if (objDIALOG)
        GUI_DialogDrawBackground(x, y, w, h);
    else 
        _gui_draw_background(x, y, w, h);
}

struct guiObject *GUI_IsModal(void)
{
    struct guiObject *obj = objHEAD;
    if (objDIALOG)
        return objDIALOG;
    while(obj) {
        if(OBJ_IS_MODAL(obj) && OBJ_IS_USED(obj) && ! OBJ_IS_HIDDEN(obj))
            return obj;
        obj = obj->next;
    }
    return NULL;
}

void _GUI_Redraw(struct guiObject *obj)
{
    OBJ_SET_DIRTY(obj, 1);
}

void GUI_RedrawAllObjects()
{
    struct guiObject *obj = objHEAD;
    while(obj) {
        if(! OBJ_IS_HIDDEN(obj)) {
            if (obj->Type == Scrollable && ((guiScrollable_t *)obj)->head) {
                //Redraw scrollable contents
                guiObject_t *head = objHEAD;
                objHEAD = ((guiScrollable_t *)obj)->head;
                GUI_RedrawAllObjects();
                objHEAD = head;
            }
            OBJ_SET_DIRTY(obj, 1);
        } else {
            OBJ_SET_DIRTY(obj, 0);
        }
        obj = obj->next;
    }
}

void _GUI_RedrawUnderlyingObjects(u16 x, u16 y, u16 w, u16 h)
{
    struct guiObject *obj = objHEAD;
    while(obj) {
        if(! OBJ_IS_HIDDEN(obj) &&
          (! ((obj->box.x + obj->box.width < x) ||
              (obj->box.x > x + w) ||
              (obj->box.y + obj->box.height < y) ||
              (obj->box.y > y + h)) || obj->Type == Label)) {
            if (obj->Type == Scrollable && ((guiScrollable_t *)obj)->head) {
                //Redraw scrollable contents
                guiObject_t *head = objHEAD;
                objHEAD = ((guiScrollable_t *)obj)->head;
                GUI_RedrawAllObjects();
                objHEAD = head;
            }
            OBJ_SET_DIRTY(obj, 1);
        }
        obj = obj->next;
    }
}

void GUI_HideObjects(struct guiObject *headObj, struct guiObject *modalObj)
{
    struct guiObject *obj;
    //Only start drawing from headObj(scrollable) or 1st modal if either is set
    obj = headObj ? headObj : modalObj ? modalObj : objHEAD;
    while(obj) {
        if(OBJ_IS_HIDDEN(obj) && OBJ_IS_DIRTY(obj)) {
            GUI_DrawBackground(obj->box.x, obj->box.y, obj->box.width, obj->box.height);
            OBJ_SET_DIRTY(obj, 0);
        }
        obj = obj->next;
    }
}

void _GUI_RefreshScreen(struct guiObject *headObj)
{
    static u8 dlg_active = 0;
    static u16 x, y, w, h;
    struct guiObject *modalObj = GUI_IsModal();
    struct guiObject *obj;

    if (FullRedraw) {
#ifdef DEBUG_DRAW
        printf("Full Redraw requested: %d\n", FullRedraw);
#endif
        if (modalObj && modalObj->Type == Dialog && FullRedraw != REDRAW_EVERYTHING) {
            //Handle Dialog redraw as an incremental
            FullRedraw = REDRAW_ONLY_DIRTY;
        } else {
            dlg_active = 0;
            //First we need to draw the main background
            GUI_DrawBackground(0, 0, LCD_WIDTH, LCD_HEIGHT);
            //Then we need to draw any supporting GUI
            FullRedraw = REDRAW_IF_NOT_MODAL;
            struct guiObject *obj = objHEAD;
            while(obj) {
                if(! OBJ_IS_HIDDEN(obj)) {
                    if(obj->Type == Dialog) {
                        dlg_active = 1;
                        x = obj->box.x;
                        y = obj->box.y;
                        w = obj->box.width;
                        h = obj->box.height;
                    }
                    GUI_DrawObject(obj);
                } else {
                    OBJ_SET_DIRTY(obj, 0);
                }
                obj = obj->next;
            }
            FullRedraw = REDRAW_ONLY_DIRTY;
            return;
        }
    }
    if(dlg_active && (objDIALOG == NULL)) {
        dlg_active = 0;
        GUI_DrawBackground(x, y, w, h);
        //GUI_RedrawAllObjects();
        _GUI_RedrawUnderlyingObjects(x, y, w, h);
    }
    GUI_HideObjects(headObj, modalObj);
    //Only start drawing from headObj(scrollable) or 1st modal if either is set
    obj = headObj ? headObj : modalObj ? modalObj : objHEAD;
    while(obj) {
        if(! OBJ_IS_HIDDEN(obj)) {
            if (obj->Type == Scrollable && ((guiScrollable_t *)obj)->head) {
                #if (LCD_WIDTH != 66) && (LCD_WIDTH != 24)
                //If scrollable changed first we need to draw the scrollable background
                if(OBJ_IS_DIRTY(obj)) {
                    #if (LCD_DEPTH == 16)
                    if(! ((((guiObject_t *)((guiScrollable_t *)obj)->head)->Type == Label) &&
                          (((guiLabel_t *)((guiScrollable_t *)obj)->head)->desc.style == LABEL_LISTBOX))) {
                        GUI_DrawBackground(obj->box.x, obj->box.y, obj->box.width, obj->box.height);
                    }
                    #else
                        GUI_DrawBackground(obj->box.x, obj->box.y, obj->box.width, obj->box.height);
                    #endif
                    OBJ_SET_DIRTY(obj, 0);
                }
                #endif
                //Redraw scrollable contents
                _GUI_RefreshScreen(((guiScrollable_t *)obj)->head);
            } else if(OBJ_IS_DIRTY(obj)) {
                if(OBJ_IS_TRANSPARENT(obj) || OBJ_IS_HIDDEN(obj)) {
                    GUI_DrawBackground(obj->box.x, obj->box.y, obj->box.width, obj->box.height);
                } else if(obj->Type == Dialog) {
                    dlg_active = 1;
                    x = obj->box.x;
                    y = obj->box.y;
                    w = obj->box.width;
                    h = obj->box.height;
                }
                GUI_DrawObject(obj);
            }
        }
        obj = obj->next;
    }
}

void GUI_RefreshScreen() {
    _GUI_RefreshScreen(NULL);
    LCD_ForceUpdate();
}

void GUI_DrawScreen(void)
{
#ifdef DEBUG_DRAW
    printf("DrawScreen\n");
#endif
    FullRedraw = REDRAW_EVERYTHING;
    _GUI_RefreshScreen(NULL);
    LCD_ForceUpdate();
}
    
void GUI_TouchRelease()
{
#if HAS_TOUCH
    in_touch = 1;
#endif
    if (objTOUCHED) {
        switch (objTOUCHED->Type) {
        case Button:
            GUI_TouchButton(objTOUCHED, -1);
            break;
        case Image:
            GUI_TouchImage(objTOUCHED, NULL, -1);
            break;
        case Label:
            GUI_TouchLabel(objTOUCHED, NULL, -1);
            break;
        case TextSelect:
            GUI_TouchTextSelect(objTOUCHED, NULL, -1);
            break;
        case Keyboard:
            GUI_TouchKeyboard(objTOUCHED, NULL, -1);
            break;
        case Scrollbar:
            GUI_TouchScrollbar(objTOUCHED, NULL, -1);
            break;
        default: break;
        }
#if HAS_TOUCH
        if (change_selection_on_touch) {
            if (objTOUCHED && OBJ_IS_SELECTABLE(objTOUCHED)) {
                GUI_SetSelected(objTOUCHED);
            }
        }
        in_touch = 0;
#endif
        objTOUCHED = NULL;
    }
}

u8 _GUI_CheckTouch(struct touch *coords, u8 long_press, struct guiObject *headObj)
{
    struct guiObject *modalObj = GUI_IsModal();
    struct guiObject *obj = headObj ? headObj : modalObj ? modalObj : objHEAD;
    
    while(obj) {
        if (! OBJ_IS_HIDDEN(obj)) {
            switch (obj->Type) {
            case UnknownGUI:
            case Dialog:
            case CheckBox:
            case Dropdown:
            case BarGraph:
            case Rect:
                break;
            case Button:
                if (coords_in_box(&obj->box, coords)) {
                    if (objTOUCHED && objTOUCHED != obj)
                        return 0;
                    objTOUCHED = obj;
                    return GUI_TouchButton(obj, long_press);
                }
                break;
            case Image:
                if (((guiImage_t *)obj)->callback &&
                    coords_in_box(&obj->box, coords))
                {
                    if (objTOUCHED && objTOUCHED != obj)
                        return 0;
                    objTOUCHED = obj;
                    return GUI_TouchImage(obj, coords, long_press);
                }
                break;
            case Label:
                if (((guiLabel_t *)obj)->pressCallback &&
                    coords_in_box(&obj->box, coords))
                {
                    if (objTOUCHED && objTOUCHED != obj)
                        return 0;
                    objTOUCHED = obj;
                    return GUI_TouchLabel(obj, coords, long_press);
                }
                break;
            case TextSelect:
                if(coords_in_box(&obj->box, coords)) {
                    if (objTOUCHED && objTOUCHED != obj)
                        return 0;
                    objTOUCHED = obj;
                    return GUI_TouchTextSelect(obj, coords, long_press);
                }
                break;
            case XYGraph:
                if(((guiXYGraph_t *)obj)->touch_cb && coords_in_box(&obj->box, coords)) {
                    if (objTOUCHED && objTOUCHED != obj)
                        return 0;
                    objTOUCHED = obj;
                    return GUI_TouchXYGraph(obj, coords, long_press);
                }
                break;
            case Keyboard:
                //Note that this only works because the keyboard encompasses the whole screen
                if (objTOUCHED && objTOUCHED != obj)
                    return 0;
                objTOUCHED = obj;
                return GUI_TouchKeyboard(obj, coords, long_press);
                break;
            case Scrollbar:
                if(coords_in_box(&obj->box, coords)) {
                    if (objTOUCHED && objTOUCHED != obj)
                        return 0;
                    objTOUCHED = obj;
                    return GUI_TouchScrollbar(obj, coords, long_press);
                }
                break;
            case Scrollable:
               if(coords_in_box(&obj->box, coords)) {
                   int ret = _GUI_CheckTouch(coords, long_press, ((guiScrollable_t *)obj)->head);
                   return ret;
               }
            }
        }
        obj = obj->next;
    }
    return 0;
}

u8 GUI_CheckTouch(struct touch *coords, u8 long_press)
{
#if HAS_TOUCH
    in_touch = 1;
    int ret = _GUI_CheckTouch(coords, long_press, NULL);
    in_touch = 0;
    return ret;
#else
    (void)coords;
    (void)long_press;
    return 0;
#endif
}

struct guiObject *GUI_GetNextSelectable(struct guiObject *origObj)
{
    struct guiObject *obj = origObj, *foundObj = NULL;

    if (! objHEAD)
        return NULL;
    struct guiObject *modalObj = GUI_IsModal();
    if (obj && OBJ_IS_SCROLLABLE(obj)) {
        //The current selected object is scrollable
        guiScrollable_t *scroll = GUI_FindScrollableParent(obj);
        obj = GUI_ScrollableGetNextSelectable(scroll, obj);
        if (obj)
            return obj;
        obj = (guiObject_t *)scroll; 
    }
    obj = obj ? obj->next : modalObj ? modalObj : objHEAD;
    while(obj) {
        if (! OBJ_IS_HIDDEN(obj) && OBJ_IS_SELECTABLE(obj))
        {
            if(obj->Type == Scrollable) {
                foundObj = GUI_ScrollableGetNextSelectable((guiScrollable_t *)obj, NULL);
            } else {
                foundObj = obj;
            }
            break;
        }
        obj = obj->next;
    }
    if (! foundObj && origObj) {
        return GUI_GetNextSelectable(NULL);
    }
    return foundObj;
}

struct guiObject *GUI_GetPrevSelectable(struct guiObject *origObj)
{
    struct guiObject *obj, *objLast = NULL;
    struct guiObject *modalObj;
    if (origObj && OBJ_IS_SCROLLABLE(origObj)) {
        //The current selected object is scrollable
        guiScrollable_t *scroll = GUI_FindScrollableParent(origObj);
        origObj = GUI_ScrollableGetPrevSelectable(scroll, origObj);
        if (origObj)
            return origObj;
        origObj = (guiObject_t *)scroll; 
    }
    modalObj = GUI_IsModal();
    obj = modalObj ? modalObj : objHEAD;
    while(obj) {
        if (obj == origObj)
            break;
        if (! OBJ_IS_HIDDEN(obj) && OBJ_IS_SELECTABLE(obj)) {
            objLast = obj;
        }
        obj = obj->next;
    }
    if (obj && ! objLast) {
        obj = obj->next;
        while(obj) {
            if (obj == origObj)
                break;
            if (! OBJ_IS_HIDDEN(obj) && OBJ_IS_SELECTABLE(obj)) {
                objLast = obj;
            }
            obj = obj->next;
        }
    }
    if (! objLast)
        objLast = origObj;
    if (objLast && objLast->Type == Scrollable)
        return GUI_ScrollableGetPrevSelectable((guiScrollable_t *)objLast, NULL);
    return objLast;
}

void GUI_HandleButtons(u8 enable)
{
    if (! enable)
        BUTTON_UnregisterCallback(&button_action);
    else 
        BUTTON_RegisterCallback(&button_action,
                CHAN_ButtonMask(BUT_LEFT)
                | CHAN_ButtonMask(BUT_RIGHT)
                | CHAN_ButtonMask(BUT_UP)
                | CHAN_ButtonMask(BUT_DOWN)
                | CHAN_ButtonMask(BUT_ENTER)
                | CHAN_ButtonMask(BUT_EXIT),
                BUTTON_PRESS | BUTTON_RELEASE | BUTTON_LONGPRESS | BUTTON_PRIORITY,
                handle_buttons,
                NULL);
}

unsigned GUI_ObjButton(struct guiObject *obj, u32 button, unsigned flags)
{
    //These objects dont handle buttons
    switch (obj->Type) {
      case UnknownGUI:
      case Dialog:
      case CheckBox:
      case Dropdown:
      case BarGraph:
      case Rect:
        return 0;
      default:
        break;
    }
    unsigned is_release = flags & BUTTON_RELEASE;
    unsigned is_longpress = flags & BUTTON_LONGPRESS;
    int press_type = is_release ? -1 : is_longpress ? 1 : 0;
    //TextSelect can handle Left, Right, and Enter
    if (obj->Type == TextSelect) {
        GUI_PressTextSelect(obj, button, press_type);
        return 1;
    }
    //These objects handle ENTER only
    if (CHAN_ButtonIsPressed(button, BUT_ENTER)) {
        switch (obj->Type) {
          case Button:
            objTOUCHED = obj;
            int retcode = GUI_TouchButton(obj, press_type);
            if (is_release)
                objTOUCHED = NULL;
            return retcode;
          case Image:
            objTOUCHED = obj;
            GUI_TouchImage(obj, NULL, press_type);
            if (is_release)
                objTOUCHED = NULL;
            return 1;
          case Label:
            objTOUCHED = obj;
            GUI_TouchLabel(obj, NULL, press_type);
            if (is_release)
                objTOUCHED = NULL;
            return 1;
          default:
            break;
        }
    }
    return 0;
}

unsigned GUI_GetRemappedButtons()
{
    if (USE_4BUTTON_MODE) {
        if (objACTIVE) {
            return CHAN_ButtonMask(BUT_UP) | CHAN_ButtonMask(BUT_DOWN);
        }
    }
    return 0;
}

unsigned handle_buttons(u32 button, unsigned flags, void *data)
{
    (void)data;
    //When modal, we capture all button presses
    int modalActive = GUI_IsModal() ? 1 : 0;

    if(USE_4BUTTON_MODE) {
        // IN 4 button mode, up/down can also act as left/right for TextSelect
        if (objACTIVE) {
            if (CHAN_ButtonIsPressed(button, BUT_UP)) {
                button = CHAN_ButtonMask(BUT_LEFT);
            } else if (CHAN_ButtonIsPressed(button, BUT_DOWN)) {
                button = CHAN_ButtonMask(BUT_RIGHT);
            } else if (CHAN_ButtonIsPressed(button, BUT_ENTER)) {
                if (flags & BUTTON_RELEASE)
                    objACTIVE = NULL;
            } else if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
                if (flags & BUTTON_RELEASE)
                    objACTIVE = NULL;
                return 1;
            }
        } else {
            if (objSELECTED && CHAN_ButtonIsPressed(button, BUT_ENTER) && objSELECTED->Type == TextSelect) {
                if (flags & BUTTON_RELEASE) {
                    objACTIVE = objSELECTED;
                }
                return 1;
            }
        }
    }
    //printf("Button: %08x Flags: %08x Active: %08x\n", button, flags, objACTIVE);
    if (CHAN_ButtonIsPressed(button, BUT_LEFT) ||
        CHAN_ButtonIsPressed(button, BUT_RIGHT) ||
        CHAN_ButtonIsPressed(button, BUT_ENTER))
    {
        // Widgets can only handle Left, Right and Enter
        if (objSELECTED)
             return GUI_ObjButton(objSELECTED, button, flags) || modalActive;
        return modalActive;
    }
    if (flags & (BUTTON_LONGPRESS | BUTTON_RELEASE)) {
       if ((flags & BUTTON_HAD_LONGPRESS) & (flags & BUTTON_RELEASE)) {
            //ignore long-press release
            return modalActive;
        }
        else if (CHAN_ButtonIsPressed(button, BUT_DOWN) || CHAN_ButtonIsPressed(button, BUT_UP)) 
        {
            
            struct guiObject *obj = (CHAN_ButtonIsPressed(button, BUT_DOWN) || ! objSELECTED)
                    ? GUI_GetNextSelectable(objSELECTED)
                    : GUI_GetPrevSelectable(objSELECTED);
            if (obj && obj != objSELECTED) {
                GUI_SetSelected(obj);
            }
        } else if (! (flags & BUTTON_LONGPRESS) && objSELECTED && CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            if (objDIALOG) {
                //Why doesn't the dialog handle its own buttons?
                DialogClose(objDIALOG, 0);
            } else {
                OBJ_SET_DIRTY(objSELECTED, 1);
                objSELECTED = NULL;
                if (select_notify)
                    select_notify(objSELECTED);
            }
        }
    } else if (! objSELECTED && CHAN_ButtonIsPressed(button, BUT_EXIT)) {
        // We need to tell the button handler that we will handle this press
        // But we ignore an EXIT if there is nothing selected
        return modalActive;
    }
    return 1;
}
guiObject_t *GUI_GetSelected()
{
    return objSELECTED;
}

void GUI_SetSelected(guiObject_t *obj)
{
    if (obj && obj != objSELECTED) {
        if (objSELECTED)
            OBJ_SET_DIRTY(objSELECTED, 1);
        objSELECTED = obj;
        if(select_notify)
            select_notify(obj);
        OBJ_SET_DIRTY(obj, 1);
    }
}

void GUI_SetSelectable(guiObject_t *obj, u8 selectable)
{
    OBJ_SET_SELECTABLE(obj, selectable);
    if(objSELECTED == obj && ! selectable) {
        objSELECTED = NULL;
        OBJ_SET_DIRTY(obj, 1);
    }
}

u8 GUI_IsSelectable(guiObject_t *obj)
{
    return OBJ_IS_SELECTABLE(obj);
}

u8 GUI_ObjectNeedsRedraw(guiObject_t *obj)
{
    return OBJ_IS_DIRTY(obj);
}

void GUI_DrawImageHelper(u16 x, u16 y, const struct ImageMap *map, u8 idx)
{
    LCD_DrawWindowedImageFromFile(x, y, map->file, map->width, map->height,
                                  map->x_off, map->y_off + idx * map->height);
}

void GUI_GetSize(struct guiObject *obj, int *width, int *height)
{
    *width = obj->box.width;
    *height = obj->box.height;
}

int GUI_IsEmpty()
{
    return objHEAD ? 0 : 1;
}

void GUI_Select1stSelectableObj()
{
    struct guiObject *obj = objHEAD;
    while(obj) {
        if(OBJ_IS_SELECTABLE(obj))
        {
            GUI_SetSelected(obj);
            return;
        }
        obj = obj->next;
    }
}
void GUI_SelectionNotify(void (*notify_cb)(guiObject_t *obj))
{
    select_notify = notify_cb;
}
#if HAS_TOUCH
void GUI_ChangeSelectionOnTouch(int enable)
{
    change_selection_on_touch = enable;
}
int GUI_InTouch()
{
    return in_touch;
}
#endif
