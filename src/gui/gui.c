/*
 This project is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Foobar is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "target.h"
#include "gui.h"

struct guiObject GUI_Array[128];
struct guiButton GUI_Button_Array[64];
struct guiLabel GUI_Label_Array[64];
struct guiFrame GUI_Frame_Array[16];
struct guiDialog GUI_Dialog_Array[8];

int GUI_CreateDialog(u16 x, u16 y, u16 width, u16 height, const char *title,
        const char *text, u16 titleColor, u16 fontColor,
        void (*CallBack)(int ObjID, struct guiDialogReturn),
        enum DialogType dgType)
{
    struct guiBox dgBox;
    struct guiImage dgImage;
    struct guiDialog dialog;
    struct guiObject objDG;

    dgBox.x = x;
    dgBox.y = y;
    dgBox.width = width;
    dgBox.height = height;
    dgImage.file = "dialog.bmp";
    dgImage.x_off = 0;
    dgImage.y_off = 0;
    dgBox.image = dgImage;
    objDG.CallBack = (void *) 0x1;
    objDG.Type = Dialog;
    objDG.Disabled = 0;
    objDG.Model = 1;
    objDG.box = dgBox;
    dialog.text = text;
    dialog.title = title;
    dialog.fontColor = fontColor;
    dialog.titleColor = titleColor;
    dialog.Type = dgType;
    dialog.CallBack = *CallBack;
    dialog.inuse = 1;

    int objLoc = GUI_GetFreeObj();
    int objDGLoc = GUI_GetFreeGUIObj(Dialog);
    if (objLoc == -1)
        return -1;
    if (objDGLoc == -1)
        return -1;
    objDG.GUIID = objLoc;
    objDG.TypeID = objDGLoc;
    GUI_Array[objLoc] = objDG;
    GUI_Dialog_Array[objDGLoc] = dialog;

    dialog.buttonid[0] = -1;
    dialog.buttonid[1] = -1;
    dialog.buttonid[2] = -1;
    dialog.buttonid[3] = -1;

    switch (dgType) {
    case dtOk: {
        dialog.buttonid[0] = GUI_CreateButton(((x + width) / 2) - 10,
                ((y + height) - 27), 89, 23, "    Ok", 0x0000, dgCallback);
        GUI_Array[dialog.buttonid[0]].Model = 1;
        GUI_Array[dialog.buttonid[0]].parent = objLoc;
    }
        break;
    case dtOkCancel: {

    }
        break;
    }
    GUI_Array[objLoc] = objDG;
    GUI_Dialog_Array[objDGLoc] = dialog;

    return objLoc;

}
int GUI_CreateLabel(u16 x, u16 y, const char *text, u16 fontColor)
{
    struct guiBox labelBox;
    struct guiObject objLabel;
    struct guiLabel label;

    labelBox.x = x;
    labelBox.y = y;
    labelBox.width = 10;
    labelBox.height = 10;
    objLabel.Type = Label;
    objLabel.CallBack = (void *) 0x1; /* no call back yet for labels */
    objLabel.Model = 0;
    objLabel.Disabled = 0;

    objLabel.box = labelBox;
    label.text = text;
    label.fontColor = fontColor;
    label.inuse = 1;
    int objLoc = GUI_GetFreeObj();
    int objLabelLoc = GUI_GetFreeGUIObj(Label);
    if (objLoc == -1)
        return -1;
    if (objLabelLoc == -1)
        return -1;
    objLabel.GUIID = objLoc;
    objLabel.TypeID = objLabelLoc;

    GUI_Array[objLoc] = objLabel;
    GUI_Label_Array[objLabelLoc] = label;
    return objLoc;
}
int GUI_CreateFrame(u16 x, u16 y, u16 width, u16 height, const char *image)
{
    struct guiBox frameBox;
    struct guiImage frameImage;
    struct guiFrame frame;
    struct guiObject objFrame;

    frameBox.x = x;
    frameBox.y = y;
    frameBox.width = width;
    frameBox.height = height;
    frameImage.file = image;
    frameImage.x_off = 0;
    frameImage.y_off = 0;
    frameBox.image = frameImage;
    objFrame.Type = Frame;
    objFrame.CallBack = (void *) 0x1;
    objFrame.Disabled = 0;
    objFrame.Model = 0;
    frame.inuse = 1;
    objFrame.box = frameBox;
    int objLoc = GUI_GetFreeObj();
    int objFrameLoc = GUI_GetFreeGUIObj(Frame);
    if (objLoc == -1)
        return -1;
    if (objFrameLoc == -1)
        return -1;
    objFrame.GUIID = objLoc;
    objFrame.TypeID = objFrameLoc;

    GUI_Array[objLoc] = objFrame;
    GUI_Frame_Array[objFrameLoc] = frame;
    return objLoc;

}
int GUI_CreateButton(u16 x, u16 y, u16 width, u16 height, const char *text,
        u16 fontColor, void (*CallBack)(int objID))
{
    struct guiBox buttonBox;
    struct guiImage buttonImage;
    struct guiButton button;
    struct guiObject objButton;

    buttonBox.x = x;
    buttonBox.y = y;
    buttonBox.width = width;
    buttonBox.height = height;
    buttonImage.file = "gui.bmp";
    buttonImage.x_off = 0;
    buttonImage.y_off = 0;
    buttonBox.image = buttonImage;
    objButton.Type = Button;
    objButton.CallBack = *CallBack;
    objButton.Disabled = 0;
    objButton.Model = 0;
    button.inuse = 1;
    objButton.box = buttonBox;
    button.text = text;
    LCD_GetStringDimensions((u8 *) text, &button.text_x_off,
            &button.text_y_off);
    printf("%s: (%d, %d), Box: (%d, %d -> %d, %d)\n", text, button.text_x_off,
            button.text_y_off, x, y, width, height);
    button.text_x_off = (width - button.text_x_off) / 2 + x;
    button.text_y_off = (height - button.text_y_off) / 2 + y;
    button.fontColor = fontColor;
    int objLoc = GUI_GetFreeObj();
    int objButtonLoc = GUI_GetFreeGUIObj(Button);
    if (objLoc == -1)
        return -1;
    if (objButtonLoc == -1)
        return -1;
    objButton.GUIID = objLoc;
    objButton.TypeID = objButtonLoc;

    GUI_Array[objLoc] = objButton;
    GUI_Button_Array[objButtonLoc] = button;
    return objLoc;
}
void GUI_DrawObject(int ObjID)
{
    if (GUI_Array[ObjID].CallBack != 0) {
        switch (GUI_Array[ObjID].Type) {
        case UnknownGUI:
            break;
        case Button: {
            struct guiBox buttonBox;
            struct guiImage buttonImage;
            buttonBox = GUI_Array[ObjID].box;
            buttonImage = GUI_Array[ObjID].box.image;
            LCD_DrawWindowedImageFromFile(buttonBox.x, buttonBox.y,
                    buttonImage.file, buttonBox.width, buttonBox.height,
                    buttonImage.x_off, buttonImage.y_off);
            LCD_SetFontColor(
                    GUI_Button_Array[GUI_Array[ObjID].TypeID].fontColor);
            LCD_PrintStringXY(
                    GUI_Button_Array[GUI_Array[ObjID].TypeID].text_x_off,
                    GUI_Button_Array[GUI_Array[ObjID].TypeID].text_y_off,
                    GUI_Button_Array[GUI_Array[ObjID].TypeID].text);
        }
            break;
        case Label: {
            struct guiBox labelBox;
            labelBox = GUI_Array[ObjID].box;
            LCD_SetFontColor(
                    GUI_Label_Array[GUI_Array[ObjID].TypeID].fontColor);
            LCD_PrintStringXY(labelBox.x, labelBox.y,
                    GUI_Label_Array[GUI_Array[ObjID].TypeID].text);
        }
            break;
        case Frame: {
            struct guiBox frameBox;
            struct guiImage frameImage;
            frameBox = GUI_Array[ObjID].box;
            frameImage = GUI_Array[ObjID].box.image;
            LCD_DrawWindowedImageFromFile(frameBox.x, frameBox.y,
                    frameImage.file, frameBox.width, frameBox.height,
                    frameImage.x_off, frameImage.y_off);
        }
            break;
        case Dialog: {
            struct guiBox dgBox;
            struct guiImage dgImage;
            dgBox = GUI_Array[ObjID].box;
            dgImage = GUI_Array[ObjID].box.image;
            LCD_DrawWindowedImageFromFile(dgBox.x, dgBox.y, dgImage.file,
                    dgBox.width, dgBox.height, dgImage.x_off, dgImage.y_off);
            LCD_SetFontColor(
                    GUI_Dialog_Array[GUI_Array[ObjID].TypeID].titleColor);
            LCD_PrintStringXY(dgBox.x + 5, (dgBox.y + 10),
                    GUI_Dialog_Array[GUI_Array[ObjID].TypeID].title);
            LCD_SetFontColor(
                    GUI_Dialog_Array[GUI_Array[ObjID].TypeID].fontColor);
            LCD_PrintStringXY(dgBox.x, (dgBox.y + ((dgBox.height / 2) - 4)),
                    GUI_Dialog_Array[GUI_Array[ObjID].TypeID].text);
        }
            break;
        case CheckBox:
            break;
        case Dropdown:
            break;
        }
    }

}
void GUI_DrawObjects(void)
{

    int i;
    for (i = 0; i < 128; i++) {
        GUI_DrawObject(i);
    }
}
void GUI_RemoveObj(int objID)
{
    struct guiObject blankObj;
    blankObj.CallBack = 0;
    blankObj.GUIID = 0;
    blankObj.Type = 0;
    blankObj.TypeID = 0;

    switch (GUI_Array[objID].Type) {
    case UnknownGUI:
        break;
    case Button: {
        struct guiButton blankButton;
        blankButton.text = " ";
        blankButton.inuse = 0;
        GUI_Button_Array[GUI_Array[objID].TypeID] = blankButton;
    }
        break;
    case Label: {
        struct guiLabel blankLabel;
        blankLabel.text = " ";
        blankLabel.inuse = 0;
        GUI_Label_Array[GUI_Array[objID].TypeID] = blankLabel;
    }
        break;
    case Frame: {
        struct guiFrame blankFrame;
        blankFrame.inuse = 0;
        GUI_Frame_Array[GUI_Array[objID].TypeID] = blankFrame;
    }
        break;
    case Dialog: {
        struct guiDialog blankDialog;
        blankDialog.buttonid[0] = -1;
        blankDialog.buttonid[1] = -1;
        blankDialog.buttonid[2] = -1;
        blankDialog.buttonid[3] = -1;
        blankDialog.inuse = 0;
        int i;
        for (i = 0; i < 4; i++) {
            GUI_RemoveObj(
                    GUI_Dialog_Array[GUI_Array[objID].TypeID].buttonid[i]);
        }

        GUI_Dialog_Array[GUI_Array[objID].TypeID] = blankDialog;
    }
        break;
    case CheckBox:
        break;
    case Dropdown:
        break;
    }
    GUI_Array[objID] = blankObj;
}

int GUI_GetFreeObj(void)
{
    int i;
    for (i = 0; i < 256; i++) {
        if (GUI_Array[i].CallBack == 0) {
            return i;
        }
    }
    return -1;
}

int GUI_GetFreeGUIObj(enum GUIType guiType)
{
    int i;
    for (i = 0; i < 256; i++) {
        switch (guiType) {
        case UnknownGUI:
            break;
        case Button: {
            if (GUI_Button_Array[i].inuse == 0) {
                return i;
            }
        }
            break;
        case Label: {
            if (GUI_Label_Array[i].inuse == 0) {
                return i;
            }
        }
            break;
        case Frame: {
            if (GUI_Frame_Array[i].inuse == 0) {
                return i;
            }
        }
            break;
        case Dialog: {
            if (GUI_Dialog_Array[i].inuse == 0) {
                return i;
            }
        }
            break;
        case CheckBox: {
        }
            break;
        case Dropdown: {
        }
            break;
        }
    }
    return -1;
}

void GUI_DrawScreen(void)
{
    /*
     * First we need to draw the main background
     *  */
    LCD_DrawWindowedImageFromFile(0, 0, "devo8.bmp", 0, 0, 0, 0);
    /*
     * Then we need to draw any supporting GUI
     */
    GUI_DrawObjects();
}
u8 GUI_CheckModel(void)
{
    int i;
    for (i = 0; i < 128; i++) {
        struct guiObject currentObject = GUI_Array[i];
        if ((currentObject.Model > 0) && (currentObject.Disabled == 0)) {
            return 1;
        }
    }
    return 0;
}
void dgCallback(int ObjID)
{
    struct guiDialogReturn gDR;
    struct guiDialog gd;
    gd = GUI_Dialog_Array[GUI_Array[GUI_Array[ObjID].parent].TypeID];
    if (gd.buttonid[0] == ObjID) {
        gDR.buttonPushed = 0;
    }
    if (gd.buttonid[1] == ObjID) {
        gDR.buttonPushed = 1;
    }
    if (gd.buttonid[2] == ObjID) {
        gDR.buttonPushed = 2;
    }
    if (gd.buttonid[3] == ObjID) {
        gDR.buttonPushed = 3;
    }
    gDR.intInput = 0;
    sprintf(gDR.strInput, " ");
    gd.CallBack(GUI_Array[ObjID].parent, gDR);
}
void GUI_DrawWindow(int ObjID)
{
    int i;
    // First we need to get the info on what we are working with
    struct guiObject obj = GUI_Array[ObjID];
    // Now we can redraw the main background...
    if (obj.Type == Label) {
        LCD_GetStringDimensions(GUI_Label_Array[obj.TypeID].text,
                &obj.box.width, &obj.box.height);
    }
    LCD_DrawWindowedImageFromFile(obj.box.x, obj.box.y, "devo8.bmp",
            obj.box.width, obj.box.height, obj.box.x, obj.box.y);

    for (i = 0; i < 128; i++) {
        if ((GUI_Array[i].CallBack != 0) && (ObjID != i)) {
            // First is this object in any way inside of the object in question...
            struct guiObject currentObj = GUI_Array[i];
            struct guiBox currentBox = currentObj.box;

            if (((currentBox.x >= obj.box.x) /* other object fully inside */
                    && ((currentBox.x + currentBox.width)
                            <= (obj.box.x + obj.box.width))
                    && (currentBox.y >= obj.box.y)
                    && ((currentBox.y + currentBox.height)
                            <= (obj.box.y + obj.box.height)))
                    || ((obj.box.x >= currentBox.x) /* Inside of other object */
                            && ((obj.box.x + obj.box.width)
                                    <= (currentBox.x + currentBox.width))
                            && (obj.box.y >= currentBox.y)
                            && ((obj.box.y + obj.box.height)
                                    <= (currentBox.y + currentBox.height))))
            {
                printf(
                        "Partial Hit...TYPE: %d ID: %d X: %d Y: %d: Width: %d Height: %d\n              TYPE: %d ID: %d X: %d Y: %d: Width: %d Height: %d\n",
                        obj.Type, ObjID, obj.box.x, obj.box.y, obj.box.width,
                        obj.box.height, currentObj.Type, i, currentBox.x,
                        currentBox.y, currentBox.width, currentBox.height);
            } else {
                printf(
                        "No Hit...TYPE: %d ID: %d X: %d Y: %d: Width: %d Height: %d\n         TYPE: %d ID: %d X: %d Y: %d: Width: %d Height: %d\n",
                        obj.Type, ObjID, obj.box.x, obj.box.y, obj.box.width,
                        obj.box.height, currentObj.Type, i, currentBox.x,
                        currentBox.y, currentBox.width, currentBox.height);
            }
        }
    }
    GUI_DrawObject(ObjID);
}
u8 GUI_CheckTouch(struct touch coords)
{
    int i;
    u8 redraw = 0;
    u8 modelActive;
    modelActive = GUI_CheckModel();
    for (i = 0; i < 128; i++) {
        struct guiObject currentObject = GUI_Array[i];
        if ((currentObject.CallBack != 0) && (currentObject.Disabled == 0)
                && ((modelActive == 0) || (currentObject.Model > 0)))
        {
            switch (currentObject.Type) {
            case UnknownGUI:
                break;
            case Button: {
                if (coords.x >= currentObject.box.x
                        && coords.x
                                <= (currentObject.box.width
                                        + currentObject.box.x)
                        && coords.y >= currentObject.box.y
                        && coords.y
                                <= (currentObject.box.height
                                        + currentObject.box.y))
                {
                    currentObject.CallBack(i);
                    redraw = 1;
                }
            }
                break;
            case Dialog:
                break; /* Dialogs are handled by buttons */
            case Label:
                break;
            case Frame:
                break;
            case CheckBox:
                break;
            case Dropdown:
                break;
            }
        }
    }
    return redraw;
}

