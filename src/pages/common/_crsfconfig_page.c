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

#if SUPPORT_CRSF_CONFIG

#include "crsf.h"

crsf_device_t crsf_devices[CRSF_MAX_DEVICES];

static struct crsfconfig_page * const mp = &pagemem.u.crsfconfig_page;
static struct crsfconfig_obj * const gui = &gui_objs.u.crsfconfig;
static u16 current_selected = 0;
static u8 number_of_devices;    // total known


static u8 CRSF_number_of_devices() {
    int i;
    for (i=0; i < CRSF_MAX_DEVICES; i++)
        if (crsf_devices[i].address == 0) break;

    return i;
}


static void press_cb(struct guiObject *obj, s8 press_type, const void *data)
{
    (void)obj;
    if (press_type != -1) {
        return;
    }
    if ((intptr_t)data < CRSF_number_of_devices())
        PAGE_PushByID(PAGEID_CRSFDEVICE, (intptr_t)data);
}

static const char *crsfconfig_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int idx = (intptr_t)data;

    return crsf_devices[idx].address ? crsf_devices[idx].name : "";
}

void PAGE_CRSFConfigEvent()
{
    u8 device_count = CRSF_number_of_devices();
    if (number_of_devices != device_count) {
        number_of_devices = device_count;
        for (int i=0; i < device_count; i++)
            GUI_Redraw(&gui->name[i]);
    }
}

#endif
