/*
    This    project    is    free    software:    you    can    redistribute    it    and/or    modify
    it    under    the    terms    of    the    GNU    General    Public    License    as    published    by
    the    Free    Software    Foundation,    either    version    3    of    the    License,    or
    (at    your    option)    any    later    version.

    Deviation    is    distributed    in    the    hope    that    it    will    be    useful,
    but    WITHOUT    ANY    WARRANTY;    without    even    the    implied    warranty    of
    MERCHANTABILITY    or    FITNESS    FOR    A    PARTICULAR    PURPOSE.        See    the
    GNU    General    Public    License    for    more    details.

    You    should    have    received    a    copy    of    the    GNU    General    Public    License
    along    with    Deviation.        If    not,    see    <http://www.gnu.org/licenses/>.
    */
#include    "common.h"
#include    "pages.h"
#include    "gui/gui.h"
#include    "config/tx.h"
#include    "config/model.h"
#include    <stdlib.h>

static    void    refresh_page_info(u8    ignore_labels);
static    u8    action_cb(u32    button,    u8    flags,    void    *data);
static    void    bind();

enum    protocol_items    {
    protocol_item    =    0,    channel_item,    fixedId_item
};

enum    display_items    {
    backlight_item    =    0,    contrast_item
};

static    struct    multi_items_config_page    *    const    mcp    =    &pagemem.u.multi_items_config_page;
void    PAGE_MultiItemsConfigInit(int    configType)
{
    mcp->config_type    =    configType;
                PAGE_SetModal(0);
                PAGE_SetActionCB(action_cb);

                mcp->buttonObj    =    NULL;
                refresh_page_info(0);
                u8    y    =    1;
                int    i;
                struct    LabelDesc    labelDesc;
    labelDesc.font    =    DEFAULT_FONT.font;
    labelDesc.font_color    =    0xffff;
    labelDesc.style    =    LABEL_CENTER;        //    set    selected    will    invert    the    color    aumatically
    labelDesc.outline_color    =    1;
    labelDesc.fill_color    =    labelDesc.outline_color;    //    not    to    draw    box
                for    (i    =    0;    i    <    mcp->item_count;    i++)    {
                    GUI_CreateLabelBox(0,    y,    0,    0,    &DEFAULT_FONT,    NULL,    NULL,        (const    void    *)mcp->label[i]);
                    mcp->itemObj[i]    =    GUI_CreateLabelBox(70,    y,    50,    MENU_ITEM_HEIGHT,    &labelDesc,    NULL,    NULL    ,
                            (const    void    *)mcp->item_content[i]);
                    GUI_SetSelectable(mcp->itemObj[i],    1);
                    y    +=    MENU_ITEM_HEIGHT;
                }
    GUI_SetSelected(mcp->itemObj[0]);
}


void    PAGE_MultiItemsConfigExit()
{
}

void    refresh_page_info(u8    ignore_labels)    {
    u8    i    =    0;
    switch    (mcp->config_type)    {
    case    protocol:
        if    (!ignore_labels)
            strncpy(mcp->label[i],    _tr("Procotol:"),    sizeof(mcp->label[i]));
        strncpy(mcp->item_content[i],    ProtocolNames[Model.protocol],    sizeof(mcp->item_content[i]));
        i++;
        if    (!ignore_labels)
            strncpy(mcp->label[i],    _tr("# Channels:"),    sizeof(mcp->label[i]));
        sprintf(mcp->item_content[i],    "%d",    Model.num_channels);
        i++;
        if    (!ignore_labels)
            strncpy(mcp->label[i],    _tr("Fixed ID:"),    sizeof(mcp->label[i]));
        if(Model.fixed_id    ==    0)
            strncpy((char    *)mcp->item_content[i],    _tr("None"),    sizeof(mcp->tmpstr));
        else
            sprintf((char    *)mcp->item_content[i],    "%d",    (int)Model.fixed_id);

        u8    y    =    MENU_ITEM_HEIGHT    *    3    +    2;
        if    (mcp->buttonObj    ==    NULL)    {    //    Do    not    create    button    more    than    once
            mcp->buttonObj    =    GUI_CreateButton((LCD_WIDTH    -    32)/2,    y,    BUTTON_DEVO10,    NULL,    0x0000,
                    NULL,    (void    *)_tr("Bind"));
            GUI_SetSelectable(mcp->buttonObj,    1);
        }
        break;
    case    display:
        if    (!ignore_labels)
            strncpy(mcp->label[i],    _tr("Backlight:"),    sizeof(mcp->label[i]));
        sprintf((char    *)mcp->item_content[i],    "%d",    (int)Transmitter.brightness);
        i++;
        if    (!ignore_labels)
            strncpy(mcp->label[i],    _tr("Contrast:"),    sizeof(mcp->label[i]));
        sprintf((char    *)mcp->item_content[i],    "%d",    (int)Transmitter.contrast);
        break;
    }
    mcp->item_count    =    i    +    1;
}

static    void    keyboard_done_cb(guiObject_t    *obj,    void    *data)
{
    (void)obj;
    (void)data;
    GUI_RemoveObj(mcp->keyboardObj);
    if    (mcp->callback_result    ==    1)    {
        if    (mcp->config_type    ==    protocol)
            Model.fixed_id    =    atoi((char    *)mcp->tmpstr);
        refresh_page_info(1);
        GUI_Redraw(mcp->item_content[fixedId_item]);
    }
}

static    void    navigate_item(short    direction)
{
    //    navigate    options    of    a    object
    u8    select    =    0;
    guiObject_t    *obj    =    GUI_GetSelected();
    switch    (mcp->config_type)    {
    case    protocol:
        if    (obj    ==    mcp->itemObj[protocol_item])    {
            select    =    Model.protocol    +    direction;
            if    (select    ==    0)    select    =    PROTOCOL_COUNT    -1;    //    rewind
            else    if    (select    >=    PROTOCOL_COUNT)    select    =    1;
            Model.protocol    =    select;
            Model.num_channels=    PROTOCOL_NumChannels();
            GUI_Redraw(mcp->itemObj[channel_item]);        //    redraw    channel    item    as    well
        }    else    if    (obj    ==    mcp->itemObj[channel_item])    {
            u8    max    =    PROTOCOL_NumChannels();
            select    =    Model.num_channels    +    direction;
            if    (select    ==    0)    select    =    max;    //    rewind
            else    if    (select    >    max)    select    =    1;
            Model.num_channels    =    select;
        }    else    if    (obj    ==    mcp->itemObj[fixedId_item])    {
            if(Model.fixed_id    ==    0)
                strncpy((char    *)mcp->tmpstr,    "0",    sizeof(mcp->tmpstr));
            else
                sprintf((char    *)mcp->tmpstr,    "%d",    (int)Model.fixed_id);
            mcp->keyboardObj    =    GUI_CreateKeyboard(KEYBOARD_NUM,    mcp->tmpstr,    999999,
                    keyboard_done_cb,    (void    *)&mcp->callback_result);
        }
        break;
    case    display:
        if    (obj    ==    mcp->itemObj[backlight_item])    {
            select    =    Transmitter.brightness    +    direction;
            if    (select    ==    255)    select    =    9;    //    rewind
            else    if    (select    >=    10)    select    =    0;
            Transmitter.brightness    =    select;
            BACKLIGHT_Brightness(Transmitter.brightness);
        }    else    if    (obj    ==    mcp->itemObj[contrast_item])    {
            select    =    Transmitter.contrast    +    direction;
            if    (select    ==    255)    select    =    9;    //    rewind
            else    if    (select    >=    10)    select    =    0;
            Transmitter.contrast    =    select;
                        int    contrast    =    0x20    +    Transmitter.contrast    *    0xC/9;
                        LCD_set_contrast(contrast);
        }
        break;
    }
    refresh_page_info(1);
    GUI_Redraw(obj);
}

u8    action_cb(u32    button,    u8    flags,    void    *data)
{
    (void)data;
    if    ((flags    &    BUTTON_PRESS)    ||    (flags    &    BUTTON_LONGPRESS))    {
        if    (CHAN_ButtonIsPressed(button,    BUT_EXIT))    {
            PAGE_ChangeByName("SubMenu",    sub_menu_item);    //    sub_menu_page    is    defined    in    sub_menu.c
        }    else    if    (CHAN_ButtonIsPressed(button,    BUT_RIGHT))    {
            navigate_item(-1);
        }        else    if    (CHAN_ButtonIsPressed(button,    BUT_LEFT))    {
            navigate_item(1);
        }        else    if    (CHAN_ButtonIsPressed(button,    BUT_UP))    {
            guiObject_t    *obj    =    GUI_GetSelected();
            GUI_SetSelected((guiObject_t    *)GUI_GetPrevSelectable(obj));
        }        else    if    (CHAN_ButtonIsPressed(button,    BUT_DOWN))    {
            guiObject_t    *obj    =    GUI_GetSelected();
            GUI_SetSelected((guiObject_t    *)GUI_GetNextSelectable(obj));
        }    else    if    (CHAN_ButtonIsPressed(button,    BUT_ENTER)){
            guiObject_t    *obj    =    GUI_GetSelected();
            if    (obj    ==    mcp->buttonObj)    {
                if    (mcp->config_type    ==    protocol)
                    bind();
            }    else    {
                GUI_SetSelected((guiObject_t    *)GUI_GetNextSelectable(obj));
            }
        }    else    {
            return    0;    //    to    let    press    call    back    to    handle    the    ENT    key
        }
    }
    return    1;
}

void    bind()
{
    if    (PROTOCOL_AutoBindEnabled())
        PROTOCOL_Init(0);
    else
        PROTOCOL_Bind();        //    bind    command    is    done        bind()    of    devo.c    or    initialize    of    dsm2.c
}

