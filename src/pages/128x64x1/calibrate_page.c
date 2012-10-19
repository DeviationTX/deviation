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
#include "config/tx.h"
#include "config/model.h"

#define CALIBRATION_TIMEOUT_MS   15000

static u8 action_cb(u32 button, u8 flags, void *data);
static const char *label_cb(guiObject_t *obj, const void *data);
static struct calibrate_page * const cp = &pagemem.u.calibrate_page;
enum calibrateState {
    notStart,
    center,
    maxmin,
    success,
    timeout
};

static enum calibrateState calibrate_state = notStart;

void PAGE_CalibrateInit(int page)
{
    (void)page;
    PAGE_SetModal(0);
    BUTTON_RegisterCallback(&cp->action,
          CHAN_ButtonMask(BUT_ENTER)
          | CHAN_ButtonMask(BUT_EXIT),
          BUTTON_PRESS | BUTTON_LONGPRESS | BUTTON_RELEASE | BUTTON_PRIORITY, action_cb, NULL);

    snprintf(cp->tmpstr, 60, _tr("Press ENT to start\ncalibration"));
    for (u8 i = 0; i < 4; i++) {
		Transmitter.calibration[i].max = 0x0000;
		Transmitter.calibration[i].min = 0xFFFF;
	}
    struct LabelDesc labelDesc;
	labelDesc.font = DEFAULT_FONT.font;
	labelDesc.font_color = 0xffff;
	labelDesc.style = CENTER;
	labelDesc.outline_color = 1;
	labelDesc.fill_color = labelDesc.outline_color; // not to draw box
    u8 row = 8;
    cp->label = GUI_CreateLabelBox(1, row, LCD_WIDTH-1, 50, &labelDesc, label_cb, NULL, "");
    calibrate_state = notStart;
}

u8 action_cb(u32 button, u8 flags, void *data)
{
	(void)data;
	u8 i;
	if (flags & BUTTON_PRESS) {
		if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
			if (calibrate_state == center || calibrate_state == maxmin) {
				PROTOCOL_Init(0);
			}
			PAGE_ChangeByName("SubMenu", 2);
		} else if (CHAN_ButtonIsPressed(button, BUT_ENTER)) {
			switch (calibrate_state){
			case timeout:
			case success:
				snprintf(cp->tmpstr, 60, _tr("Press ENT to start\ncalibration"));
				GUI_Redraw(cp->label);
				calibrate_state = notStart;
				break;
			case notStart:
				calibrate_state = center;
				PROTOCOL_DeInit();
				snprintf(cp->tmpstr, 60, _tr("Center all sticks\nthen press ENT"));
				GUI_Redraw(cp->label);
				break;
			case center:
				for (i = 0; i < 4; i++) {
					s32 value = CHAN_ReadRawInput(i + 1);
					Transmitter.calibration[i].zero = value;
				}
				snprintf(cp->tmpstr, 60, _tr("Move sticks\nto max & min\nthen press ENT"));
				GUI_Redraw(cp->label);
				calibrate_state = maxmin;
				break;
			case maxmin:
				calibrate_state = success;
				snprintf(cp->tmpstr, 60, _tr("Calibration Success"));
				GUI_Redraw(cp->label);
				for (i = 0; i < 4; i++) {
					printf("Input %d: Max: %d Min: %d Zero: %d\n", i+1, Transmitter.calibration[i].max, Transmitter.calibration[i].min, Transmitter.calibration[i].zero);
				}
				PROTOCOL_Init(0);
				break;
			}
		}else {
			// only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
			return 0;
		}
	}
	return 1;
}

void PAGE_CalibrateEvent()
{
	if (calibrate_state == maxmin) {
		u32 calibrationTimeoutScan = CLOCK_getms() + CALIBRATION_TIMEOUT_MS;
		while (calibrate_state == maxmin) {
			CLOCK_ResetWatchdog();
			if(PWR_CheckPowerSwitch())
				PWR_Shutdown();
			BUTTON_Handler();
			GUI_RefreshScreen();
			if (CLOCK_getms() > calibrationTimeoutScan) {
				// fail to press ENT within given timeout
				calibrate_state = timeout;
				PROTOCOL_Init(0);
				snprintf(cp->tmpstr, 60, _tr("Calibrate failed\n:timeout"));
				GUI_Redraw(cp->label);
				break;
			}
			for (u8 i = 0; i < 4; i++) {
				s32 value = CHAN_ReadRawInput(i + 1);
				if (value > Transmitter.calibration[i].max)
					Transmitter.calibration[i].max = value;
				else if (value < Transmitter.calibration[i].min)
					Transmitter.calibration[i].min = value;
			}
		}
		CLOCK_ResetWatchdog();
		if(PWR_CheckPowerSwitch())
			PWR_Shutdown();
	}
}

void PAGE_CalibrateExit()
{
	BUTTON_UnregisterCallback(&cp->action);
}

const char *label_cb(guiObject_t *obj, const void *data)
{
	(void)obj;
	(void)data;
    return cp->tmpstr;
}


