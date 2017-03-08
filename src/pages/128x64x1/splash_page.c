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

static struct splash_obj * const gui = &gui_objs.u.splash;

static unsigned _action_cb(u32 button, unsigned flags, void *data);
//static int offset =  110;

void PAGE_SplashInit(int page)
{
    (void)page;
    if(Transmitter.splash_delay == 0) {
        PAGE_ChangeByID(PAGEID_MAIN, 0);
        return;
    }
    PAGE_RemoveAllObjects();
    PAGE_SetActionCB(_action_cb);
    u16 w, h;
    LCD_ImageDimensions(SPLASH_FILE, &w, &h);
    //GUI_CreateImageOffset(&gui->splash_image, 15, 10, w-offset, h-5, offset, 0, SPLASH_FILE, NULL, NULL);
    if( w < LCD_WIDTH - 1 && h < LCD_HEIGHT - LINE_HEIGHT - 3) 
    	GUI_CreateImageOffset(&gui->splash_image, (LCD_WIDTH-w)/2, (LCD_HEIGHT-h-LINE_HEIGHT)/2, w, h, 0, 0, SPLASH_FILE, NULL, NULL);
    GUI_CreateLabelBox(&gui->version, 0, LCD_HEIGHT - LINE_HEIGHT - 1 , LCD_WIDTH, LINE_HEIGHT, &TINY_FONT, NULL, NULL, DeviationVersion);
}

static unsigned _action_cb(u32 button, unsigned flags, void *data)
{
    (void)data;
    (void)button;
    (void)flags;
    PAGE_ChangeByID(PAGEID_MAIN, 0);
    return 1;
}

void PAGE_SplashEvent()
{
    static unsigned int time=0;
    if(GUI_IsModal())
       return;
//    u8 step = 5;
    if ( 0 == time )
    	time = CLOCK_getms() + Transmitter.splash_delay * 100;
    if ( CLOCK_getms() > time ) 
	PAGE_ChangeByID(PAGEID_MAIN,0);
/*     if ( offset > 0 ) {
	offset -= step;
	GUI_ChangeImage(&gui->splash_image,SPLASH_FILE,offset,0);
	GUI_Redraw(&gui->splash_image);
    }*/
}

void PAGE_SplashExit()
{
}
