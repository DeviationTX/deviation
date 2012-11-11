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

struct guiObject *objHEAD     = NULL;
struct guiObject *objTOUCHED  = NULL;
struct guiObject *objSELECTED = NULL;
struct guiObject *objModalButton = NULL;

static struct guiObject GUI_Array[100];
static buttonAction_t button_action;
static buttonAction_t button_modalaction;
static u8 FullRedraw;

static u8 handle_buttons(u32 button, u8 flags, void*data);
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
}

u8 coords_in_box(struct guiBox *box, struct touch *coords)
{
    return(coords->x >= box->x && coords->x <= (box->x + box->width)
        && coords->y >= box->y && coords->y <= (box->y + box->height));
}

void GUI_DrawObject(struct guiObject *obj)
{
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
    case Listbox:    GUI_DrawListbox(obj, 1);        break;
    case Keyboard:   GUI_DrawKeyboard(obj);          break;
    case Scrollbar:  GUI_DrawScrollbar(obj);         break;
    case Rect:       GUI_DrawRect(obj);              break;
    }
    if (obj == objSELECTED)
        _gui_hilite_selected(obj);
    OBJ_SET_DIRTY(obj, 0);
}

void GUI_DrawObjects(void)
{

    struct guiObject *obj = objHEAD;
    while(obj) {
        if(! OBJ_IS_HIDDEN(obj))
        {
            GUI_DrawObject(obj);
        } else {
            OBJ_SET_DIRTY(obj, 0);
        }
        obj = obj->next;
    }
}

void GUI_RemoveHierObjects(struct guiObject *obj)
{
    struct guiObject *parent = objHEAD;
    if(obj == objHEAD) {
        GUI_RemoveAllObjects();
        return;
    }
    while(parent && parent->next != obj)
        parent = parent->next;
    if(! parent)
        return;
    while(parent->next)
        GUI_RemoveObj(parent->next);
    FullRedraw = 1;
}

void GUI_RemoveAllObjects()
{
    while(objHEAD)
        GUI_RemoveObj(objHEAD);
    FullRedraw = 1;
}

void GUI_RemoveObj(struct guiObject *obj)
{
    switch(obj->Type) {
    case Dialog: {
        GUI_HandleModalButtons(0);
        GUI_RemoveHierObjects(obj->next);
        break;
    }
    case Scrollbar:
        BUTTON_UnregisterCallback(&obj->o.scrollbar.action);
        break;
    case Keyboard:
        BUTTON_UnregisterCallback(&obj->o.keyboard.action);
        break;
    case Listbox:
        BUTTON_UnregisterCallback(&obj->o.listbox.action);
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
    FullRedraw = 1;
}

void GUI_SetHidden(struct guiObject *obj, u8 state)
{
    if((!!state) == (!! OBJ_IS_HIDDEN(obj)))
        return;
    OBJ_SET_HIDDEN(obj, state);
    OBJ_SET_DIRTY(obj, 1);
}

struct guiObject *GUI_GetFreeObj(void)
{
    int i;
    struct guiObject *obj;
    for (i = 0; i < 256; i++) {
        if (! OBJ_IS_USED(&GUI_Array[i])) {
            obj = &GUI_Array[i];
            obj->next = NULL;
            obj->flags= 0;
            OBJ_SET_DIRTY(obj, 1);
            OBJ_SET_USED(obj, 1);
            return obj;
        }
    }
    return NULL;
}

void GUI_DrawBackground(u16 x, u16 y, u16 w, u16 h)
{
    if(w == 0 || h == 0)
        return;
    if(FullRedraw && w != LCD_WIDTH//SCREEN_WIDTH
            && h != LCD_HEIGHT) //SCREEN_HEIGHT)
        return;  //Optimization to prevent partial redraw when it isn't needed
    _gui_draw_background(x, y, w, h);
}

void GUI_DrawScreen(void)
{
    /*
     * First we need to draw the main background
     *  */
    GUI_DrawBackground(0, 0, LCD_WIDTH, LCD_HEIGHT);//320, 240);
    /*
     * Then we need to draw any supporting GUI
     */
    FullRedraw = 1;
    GUI_DrawObjects();
    FullRedraw = 0;
}

struct guiObject *GUI_IsModal(void)
{
    struct guiObject *obj = objHEAD;
    while(obj) {
        if(OBJ_IS_MODAL(obj) && OBJ_IS_USED(obj) && ! OBJ_IS_HIDDEN(obj))
            return obj;
        obj = obj->next;
    }
    return NULL;
}

void GUI_Redraw(struct guiObject *obj)
{
    OBJ_SET_DIRTY(obj, 1);
}

void GUI_RedrawAllObjects()
{
    FullRedraw = 1;
}

void GUI_HideObjects(struct guiObject *modalObj)
{
    struct guiObject *obj;
    obj = objHEAD;
    while(obj) {
        if(OBJ_IS_HIDDEN(obj) && OBJ_IS_DIRTY(obj) && (! modalObj || OBJ_IS_MODAL(obj))) {
            if (modalObj && modalObj->Type == Dialog) {
                GUI_DialogDrawBackground(obj->box.x, obj->box.y, obj->box.width, obj->box.height);
            } else {
                GUI_DrawBackground(obj->box.x, obj->box.y, obj->box.width, obj->box.height);
            }
            OBJ_SET_DIRTY(obj, 0);
        }
        obj = obj->next;
    }
}

void GUI_RefreshScreen(void)
{
    struct guiObject *obj;
    if (FullRedraw) {
        GUI_DrawScreen();
        return;
    }
    struct guiObject *modalObj = GUI_IsModal();
    GUI_HideObjects(modalObj);
    obj = objHEAD;
    while(obj) {
        if(! OBJ_IS_HIDDEN(obj) && OBJ_IS_DIRTY(obj) && (! modalObj || OBJ_IS_MODAL(obj))) {
            if(OBJ_IS_TRANSPARENT(obj) || OBJ_IS_HIDDEN(obj)) {
                if (modalObj && modalObj->Type == Dialog) {
                    GUI_DialogDrawBackground(obj->box.x, obj->box.y, obj->box.width, obj->box.height);
                } else {
                    GUI_DrawBackground(obj->box.x, obj->box.y, obj->box.width, obj->box.height);
                }
            }
            GUI_DrawObject(obj);
        }
        obj = obj->next;
    }
}

void GUI_TouchRelease()
{
    if (objTOUCHED) {
        switch (objTOUCHED->Type) {
        case Button:
          {
            struct guiButton *button = &objTOUCHED->o.button;
            OBJ_SET_DIRTY(objTOUCHED, 1);
            if(button->CallBack)
                button->CallBack(objTOUCHED, button->cb_data);
            break;
          }
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
        objTOUCHED = NULL;
    }
}

u8 GUI_CheckTouch(struct touch *coords, u8 long_press)
{
    u8 modalActive;
    modalActive = GUI_IsModal() ? 1 : 0;
    struct guiObject *obj = objHEAD;
    
    while(obj) {
        if (! OBJ_IS_HIDDEN(obj)
            && ((modalActive == 0) || OBJ_IS_MODAL(obj)))
        {
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
                    OBJ_SET_DIRTY(obj, 1);
                    return 1;
                }
                break;
            case Image:
                if (obj->o.image.callback &&
                    coords_in_box(&obj->box, coords))
                {
                    if (objTOUCHED && objTOUCHED != obj)
                        return 0;
                    objTOUCHED = obj;
                    return GUI_TouchImage(obj, coords, long_press);
                }
                break;
            case Label:
                if (obj->o.label.pressCallback &&
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
            case Listbox:
                if(coords_in_box(&obj->box, coords)) {
                    if (objTOUCHED && objTOUCHED != obj)
                        return 0;
                    objTOUCHED = obj;
                    return GUI_TouchListbox(obj, coords, long_press);
                }
                break;
            case XYGraph:
                if(coords_in_box(&obj->box, coords)) {
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
            }
        }
        obj = obj->next;
    }
    return 0;
}

struct guiObject *GUI_GetNextSelectable(struct guiObject *origObj)
{
    struct guiObject *obj = origObj, *foundObj = NULL;

    if (! objHEAD)
        return NULL;
    u8 modalActive = GUI_IsModal() ? 1 : 0;
    obj = obj ? obj->next : objHEAD;
    while(obj) {
        if (! OBJ_IS_HIDDEN(obj) && OBJ_IS_SELECTABLE(obj) && (! modalActive || OBJ_IS_MODAL(obj)))
        {
            foundObj = obj;
            break;
        }
        obj = obj->next;
    }
    if (! foundObj && origObj) {
        return GUI_GetNextSelectable(NULL);
    }
    return obj;
}
struct guiObject *GUI_GetPrevSelectable(struct guiObject *origObj)
{
    struct guiObject *obj = objHEAD, *objLast = NULL;
    if (obj == NULL)
        return GUI_GetNextSelectable(objHEAD);
    u8 modalActive = GUI_IsModal() ? 1 : 0;
    while(obj) {
        if (obj == origObj)
            break;
        if (! OBJ_IS_HIDDEN(obj) && OBJ_IS_SELECTABLE(obj) && (! modalActive || OBJ_IS_MODAL(obj)))
        {
            objLast = obj;
        }
        obj = obj->next;
    }
    if (obj && ! objLast) {
        obj = obj->next;
        while(obj) {
            if (obj == origObj)
                break;
            if (! OBJ_IS_HIDDEN(obj) && OBJ_IS_SELECTABLE(obj) && (! modalActive || OBJ_IS_MODAL(obj)))
            {
                objLast = obj;
            }
            obj = obj->next;
        }
    }
    if (! objLast)
        return origObj;
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

u8 handle_buttons(u32 button, u8 flags, void *data)
{
    (void)data;
    //When modal, we capture all button presses
    u8 modalActive = GUI_IsModal() ? 1 : 0;
    if (flags & BUTTON_RELEASE) {
        if (objSELECTED && objTOUCHED == objSELECTED && (
             CHAN_ButtonIsPressed(button, BUT_LEFT) ||
             CHAN_ButtonIsPressed(button, BUT_RIGHT) ||
             CHAN_ButtonIsPressed(button, BUT_ENTER)))
        {
            //Button is emulating a touch, so send a release
            GUI_TouchRelease();
            return 1;
        } else if (flags & BUTTON_HAD_LONGPRESS) {
            //ignore long-press release
            return 0;
        } else if (CHAN_ButtonIsPressed(button, BUT_DOWN) ||
                   (! objSELECTED && CHAN_ButtonIsPressed(button, BUT_UP)))
        {
            struct guiObject *obj = GUI_GetNextSelectable(objSELECTED);
            if (obj && obj != objSELECTED) {
                if (objSELECTED)
                    OBJ_SET_DIRTY(objSELECTED, 1);
                objSELECTED = obj;
                OBJ_SET_DIRTY(obj, 1);
                return 1;
            }
        } else if (CHAN_ButtonIsPressed(button, BUT_UP)) {
            struct guiObject *obj = GUI_GetPrevSelectable(objSELECTED);
            if (obj && obj != objSELECTED) {
                if (objSELECTED)
                    OBJ_SET_DIRTY(objSELECTED, 1);
                objSELECTED = obj;
                OBJ_SET_DIRTY(obj, 1);
                return 1;
            }
        } else if (objSELECTED && CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            OBJ_SET_DIRTY(objSELECTED, 1);
            objSELECTED = NULL;
            return 1;
        }
        return modalActive;
    }
    if (! objHEAD)
        return modalActive;

    if (CHAN_ButtonIsPressed(button, BUT_DOWN) ||
        CHAN_ButtonIsPressed(button, BUT_UP) ||
        CHAN_ButtonIsPressed(button, BUT_EXIT))
    {
        return (flags & BUTTON_LONGPRESS) ? modalActive : 1;
    }
    if (CHAN_ButtonIsPressed(button, BUT_ENTER) && flags & BUTTON_LONGPRESS) {
        //long-press on enter is ignored so it can be handled by the menu
        return modalActive;
    }
    if (objSELECTED) {
        void(*press)(struct guiObject *obj, u32 button, u8 press_type) = NULL;
        if (objSELECTED->Type == TextSelect) {
            press = GUI_PressTextSelect;
        }
        //else if(objSELECTED->Type == Listbox) {  // Bug fix for issue #81, listbox should handle up/down as well
        //    press = GUI_PressListbox;
        //}
        if (CHAN_ButtonIsPressed(button, BUT_ENTER)) {
            if (! objTOUCHED || objTOUCHED == objSELECTED) {
                if (press) {
                    press(objSELECTED, BUT_ENTER, flags & BUTTON_LONGPRESS);
                    objTOUCHED = objSELECTED;
                    return 1;
                } else {
                    struct touch coords;
                    coords.x = objSELECTED->box.x + (objSELECTED->box.width >> 1);
                    coords.y = objSELECTED->box.y + (objSELECTED->box.height >> 1);
                    GUI_CheckTouch(&coords, flags & BUTTON_LONGPRESS);
                    return 1;
                }
            }
        } else if (press) {
            if (! objTOUCHED || objTOUCHED == objSELECTED) {
                if (CHAN_ButtonIsPressed(button, BUT_RIGHT)) {
                    press(objSELECTED, BUT_RIGHT, flags & BUTTON_LONGPRESS);
                    objTOUCHED = objSELECTED;
                    return 1;
                } else if (CHAN_ButtonIsPressed(button, BUT_LEFT)) {
                    press(objSELECTED, BUT_LEFT, flags & BUTTON_LONGPRESS);
                    objTOUCHED = objSELECTED;
                    return 1;
                }
            }
        }
        return 1;
    }
    return modalActive;
}
guiObject_t *GUI_GetSelected()
{
    return objSELECTED;
}

void GUI_SetSelected(guiObject_t *obj)
{
    if (obj == NULL) {
        return;
    }
    if (objSELECTED != NULL && objSELECTED != obj) { // bug fix: should set old obj dirty as well
        OBJ_SET_DIRTY(objSELECTED, 1);
    }
    objSELECTED = obj;
    OBJ_SET_DIRTY(obj, 1);
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
