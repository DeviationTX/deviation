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

#ifndef EMULATOR
crsf_device_t crsf_devices[CRSF_MAX_DEVICES];
#else
crsf_device_t crsf_devices[] = {
{
  .address = 238,
  .number_of_params = 24,
  .params_version = 1,
  .serial_number = 18661,
  .hardware_id = 65539,
  .firmware_id = 600,
  .name = "XF Transmitter",
},
{
  .address = 236,
  .number_of_params = 40,
  .params_version = 1,
  .serial_number = 29509,
  .hardware_id = 73760,
  .firmware_id = 600,
  .name = "XF Nano RX",
},
};
#endif

static struct crsfconfig_page * const mp = &pagemem.u.crsfconfig_page;
static struct crsfconfig_obj * const gui = &gui_objs.u.crsfconfig;
static u16 current_selected = 0;

static u32 last_update;
static u8 number_of_devices;    // total known


static const char *crsfconfig_str_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    int idx = (int)data;

    return crsf_devices[idx].address ? crsf_devices[idx].name : "";
}

u8 CRSF_number_of_devices() {
    int i;
    for (i=0; i < CRSF_MAX_DEVICES; i++)
        if (crsf_devices[i].address == 0) break;

    return i;
}

void PAGE_CRSFConfigEvent()
{
    if (CLOCK_getms() - last_update > 500) {
        last_update = CLOCK_getms();
        u8 device_count = CRSF_number_of_devices();
        if (number_of_devices != device_count) {
            number_of_devices = device_count;
            for (int i=0; i < device_count; i++)
                GUI_Redraw(&gui->name[i]);
        }
    }
    last_update = CLOCK_getms();
}

#endif
