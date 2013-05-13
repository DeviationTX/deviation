/*
 This project is free software: you can redistribute it and/or modify
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
#include "pages.h"
#include "gui/gui.h"

#define gui (&gui_objs.u.splash)
static u8 _action_cb(u32 button, u8 flags, void *data);
//static int offset =  110;

void PAGE_SplashInit(int page)
{
    (void)page;
    PAGE_RemoveAllObjects();
    PAGE_SetActionCB(_action_cb);
    u16 w, h;
    LCD_ImageDimensions(SPLASH_FILE, &w, &h);
    if( w < LCD_WIDTH - 10 && h < LCD_HEIGHT - 20)
    	GUI_CreateImageOffset(&gui->splash_image, (LCD_WIDTH-w)/2, (LCD_HEIGHT-h)/2, w, h, 0, 0, SPLASH_FILE, NULL, NULL);
    GUI_CreateLabelBox(&gui->version,  (LCD_WIDTH-w)/2, (LCD_HEIGHT+h)/2 + 5, 0, 0, &TINY_FONT, NULL, NULL, _tr_noop(DeviationVersion));
}

static u8 _action_cb(u32 button, u8 flags, void *data)
{
    (void)data;
    (void)button;
    (void)flags;
    PAGE_ChangeByID(PAGEID_MAIN);
    return 1;
}

void PAGE_SplashEvent()
{
    static unsigned int time=0;
    if ( 0 == time )
    	time = CLOCK_getms()+ 3500; // 3 sec.
    if ( CLOCK_getms() > time ) 
	PAGE_ChangeByID(PAGEID_MAIN);
}

void PAGE_SplashExit()
{
    PAGE_SetActionCB(NULL);
}
