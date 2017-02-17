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
#include "extended_audio.h"

static struct splash_obj * const gui = &gui_objs.u.splash;
static unsigned _action_cb(u32 button, unsigned flags, void *data);
//static int offset =  110;

void PAGE_SplashInit(int page)
{
    (void)page;
    if (Transmitter.splash_delay == 0) {
        PAGE_ChangeByID(PAGEID_MAIN, 0);
        return;
    }
    PAGE_RemoveAllObjects();
    PAGE_SetActionCB(_action_cb);
    u16 w, h;
    LCD_ImageDimensions(SPLASH_FILE, &w, &h);
    if( w < LCD_WIDTH - 10 && h < LCD_HEIGHT - 20)
    	GUI_CreateImageOffset(&gui->splash_image, (LCD_WIDTH-w)/2, (LCD_HEIGHT-h)/2, w, h, 0, 0, SPLASH_FILE, NULL, NULL);
    GUI_CreateLabelBox(&gui->version,  (LCD_WIDTH-w)/2, (LCD_HEIGHT+h)/2 + 5, 0, 0, &TINY_FONT, NULL, NULL, DeviationVersion);
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
#if HAS_EXTENDED_AUDIO
    static unsigned int time_startup_msg;
#endif
    if (GUI_IsModal())
        return;
    if ( 0 == time ) {
    	time = CLOCK_getms()+ Transmitter.splash_delay * 100; // 3 sec.
#if HAS_EXTENDED_AUDIO
        time_startup_msg = CLOCK_getms() + 5 * 100;     // Dealy 0.5 second to play startup audio
#endif
    }
#if HAS_EXTENDED_AUDIO
    if (time_startup_msg && (CLOCK_getms() > time_startup_msg) ) {
        AUDIO_SetVolume();
        MUSIC_Play(MUSIC_STARTUP);
        time_startup_msg = 0;
    }
#endif
    // We use SPITouch_IRQ() here instead of attaching an event to the image because
    // We want to abort regardless of where on the page the touch occurred
    if ( CLOCK_getms() > time || SPITouch_IRQ())
	PAGE_ChangeByID(PAGEID_MAIN, 0);
}

void PAGE_SplashExit()
{
    PAGE_SetActionCB(NULL);
}
