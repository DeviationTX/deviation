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

static struct tx_configure_page * const cp = &pagemem.u.tx_configure_page;  // MACRO is not good when debugging
static struct calibrate_obj * const guic = &gui_objs.u.calibrate;

enum {
#ifndef NO_LANGUAGE_SUPPORT
    ITEM_LANG,
#endif
    ITEM_MODE,
    ITEM_STICKS,
    ITEM_BUZZ,
    ITEM_HAPTIC,
    ITEM_PWR_ALARM,
    ITEM_BATT,
    ITEM_ALARM_INTV,
    ITEM_PWRDN_ALARM,
    ITEM_BACKLIGHT,
    ITEM_CONTRAST,
    ITEM_DIMTIME,
    ITEM_DIMVAL,
    ITEM_PREALERT,
    ITEM_PREALERT_IVAL,
    ITEM_TIMEUP,
    ITEM_TELEMTEMP,
    ITEM_TELEMLEN,
    ITEM_LAST,
};

enum calibType {
    CALIB_NONE,
    CALIB_TOUCH,
    CALIB_STICK,
    CALIB_STICK_TEST,
    SET_CLOCK,
};

static inline guiObject_t *_get_obj(int idx, int objid);
static const int XCOORD = 20;
static const int YCOORD = 20;
enum calibrateState {
    CALI_CENTER,
    CALI_MAXMIN,
    CALI_SUCCESS,
    CALI_SUCCESSEXIT,
    CALI_EXIT,
};
static enum calibrateState calibrate_state;
static const char *auto_dimmer_time_cb(guiObject_t *obj, int dir, void *data);
static unsigned _action_cb_calibrate(u32 button, unsigned flags, void *data)
{
    (void)data;
    u8 i;
    if (flags & BUTTON_PRESS)
        return 1;
    if (flags & BUTTON_RELEASE) {
        if (CHAN_ButtonIsPressed(button, BUT_EXIT)) {
            // bug fix: when most users see the "Calibration done", it is very likely tha they will press ext to exit,
            // then all calibration data are rollback, and cause the calibration failed -- what a tough bug!!
            if (calibrate_state == CALI_SUCCESS)
                calibrate_state = CALI_SUCCESSEXIT;
            else
                calibrate_state = CALI_EXIT;
        } else if (CHAN_ButtonIsPressed(button, BUT_ENTER)) {
            switch (calibrate_state){
            case CALI_CENTER:
                for (i = 0; i < INP_HAS_CALIBRATION; i++) {
                    s32 value = CHAN_ReadRawInput(i + 1);
                    Transmitter.calibration[i].max = 0x0000;
                    Transmitter.calibration[i].min = 0xFFFF;
                    Transmitter.calibration[i].zero = value;
                }
                snprintf(tempstring, sizeof(tempstring), "%s", _tr("Move sticks and knobs\nto max & min positions\nthen press ENT"));
                GUI_Redraw(&guic->msg);
                calibrate_state = CALI_MAXMIN;
                break;
            case CALI_MAXMIN:
                for (i = 0; i < INP_HAS_CALIBRATION; i++) {
                    printf("Input %d: Max: %d Min: %d Zero: %d\n", i+1, Transmitter.calibration[i].max, Transmitter.calibration[i].min, Transmitter.calibration[i].zero);
                }
                GUI_DrawBackground(0, 0, LCD_WIDTH, LCD_HEIGHT);
                snprintf(tempstring, sizeof(tempstring), "%s", _tr("Calibration done.\n \nPress ENT."));
                GUI_Redraw(&guic->msg);
                calibrate_state = CALI_SUCCESS;
                break;
            case CALI_SUCCESS:
                calibrate_state = CALI_SUCCESSEXIT;
                break;
            default: break;
            }
        }else {
            // only one callback can handle a button press, so we don't handle BUT_ENTER here, let it handled by press cb
            return 0;
        }
    }
    return 1;
}

void PAGE_CalibInit(int page)
{
    (void)page;
    PROTOCOL_DeInit();
    PAGE_SetActionCB(_action_cb_calibrate);
    snprintf(tempstring, sizeof(tempstring), "%s",  _tr("Center all \nsticks and knobs\nthen press ENT"));
    GUI_CreateLabelBox(&guic->msg, 1, CALIB_Y, 0, 0,
            LCD_HEIGHT > 70? &NARROW_FONT:&DEFAULT_FONT, NULL, NULL, tempstring);
    memcpy(cp->calibration, Transmitter.calibration, sizeof(cp->calibration));

    while(1) {
        CLOCK_ResetWatchdog();
        if(PWR_CheckPowerSwitch())
            PWR_Shutdown();
        if(priority_ready & (1 << MEDIUM_PRIORITY)) {
            BUTTON_Handler();
            priority_ready &= ~(1 << MEDIUM_PRIORITY);
        }
        if(priority_ready & (1 << LOW_PRIORITY)) {
            //Only sample every 100msec
            GUI_RefreshScreen();
            priority_ready = 0;
        }
        for (u8 i = 0; i < INP_HAS_CALIBRATION; i++) {
            s32 value = CHAN_ReadRawInput(i + 1);
            if (value > Transmitter.calibration[i].max)
                Transmitter.calibration[i].max = value;
            else if (value < Transmitter.calibration[i].min)
                Transmitter.calibration[i].min = value;
        }
        if (calibrate_state == CALI_SUCCESSEXIT || calibrate_state == CALI_EXIT)
            break;
    }
    if (calibrate_state == CALI_EXIT)
        memcpy(Transmitter.calibration, cp->calibration, sizeof(cp->calibration));

    PAGE_Pop();
//    PAGE_SetActionCB(NULL);
//    PROTOCOL_Init(0);
//    PAGE_SetModal(0);
//    //cp->enable = CALIB_NONE;
//    PAGE_ChangeByID(PAGEID_TXCFG, 0);
}

static const char *calibratestr_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    return _tr("Calibrate");
}

#if HAS_RTC
static const char *clockstr_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    (void)data;
    return _tr("Set");
}
#endif

static const char *modeselect_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    Transmitter.mode = GUI_TextSelectHelper(Transmitter.mode, MODE_1, MODE_4, dir, 1, 1, NULL);
    snprintf(tempstring, sizeof(tempstring), _tr("Mode %d"), Transmitter.mode);
    return tempstring;
}

static const char *backlight_select_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    u8 changed;
    Transmitter.backlight = GUI_TextSelectHelper(Transmitter.backlight,
                                  MIN_BRIGHTNESS, 10, dir, 1, 1, &changed);
    if (changed)
        BACKLIGHT_Brightness(Transmitter.backlight);
    if (Transmitter.backlight == 0)
        return _tr("Off");
    sprintf(tempstring, "%d", Transmitter.backlight);
    return tempstring;
}

static const char *common_select_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    u8 *unsigned_data = (u8 *)data;
    if (GUI_IsTextSelectEnabled(obj)) {
        *unsigned_data = GUI_TextSelectHelper(*unsigned_data, 0, 10, dir, 1, 1, NULL);
    }
    if (*unsigned_data == 0)
        return _tr("Off");
    sprintf(tempstring, "%d", *unsigned_data);
    return tempstring;
}

static const char *auto_dimmer_time_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    u8 changed;
    u16 dimmer_timmer = Transmitter.auto_dimmer.timer/1000;
    dimmer_timmer = GUI_TextSelectHelper(dimmer_timmer,
            MIN_BACKLIGHT_DIMTIME, MAX_BACKLIGHT_DIMTIME, dir, 5, 10, &changed);
    if (changed)
        Transmitter.auto_dimmer.timer = dimmer_timmer * 1000;
    guiObject_t *dimobj = _get_obj(ITEM_DIMVAL, 0);
    if (dimmer_timmer == 0) {
        if(dimobj)
            GUI_TextSelectEnable((guiTextSelect_t *)dimobj, 0);
        return _tr("Off");
    }
    if (dimobj)
        GUI_TextSelectEnable((guiTextSelect_t *)dimobj, 1);
    TIMER_SetString(tempstring, Transmitter.auto_dimmer.timer);
    return tempstring;
}

static const char *prealert_time_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    u8 changed;
    u16 prealert_time = Transmitter.countdown_timer_settings.prealert_time/1000;
    prealert_time = GUI_TextSelectHelper(prealert_time,
            MIN_PERALERT_TIME, MAX_PERALERT_TIME, dir, 5, 10, &changed);
    if (changed)
        Transmitter.countdown_timer_settings.prealert_time = prealert_time * 1000;
    if (prealert_time == 0)
        return _tr("Off");
    TIMER_SetString(tempstring, Transmitter.countdown_timer_settings.prealert_time);
    return tempstring;
}

static const char *timer_interval_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u16 *value = (u16 *)data;
    u8 changed;
    u16 interval = *value/1000;
    interval = GUI_TextSelectHelper(interval, 0, 60, dir, 1, 5, &changed);
    if (changed)
        *value = interval * 1000;
    if (interval == 0)
        return _tr("Off");
    sprintf(tempstring, "%d", interval);
    return tempstring;
}

static const char *poweralarm_select_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    u8 changed;
    Transmitter.power_alarm = GUI_TextSelectHelper(Transmitter.power_alarm,
            0 , MAX_POWER_ALARM, dir, 1 , 5 , &changed);
    if( 0 == Transmitter.power_alarm)
	return _tr("Off");
    sprintf(tempstring, "%2dmn", Transmitter.power_alarm);
    return tempstring;
}

static const char *batalarm_select_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    u8 changed;
    Transmitter.batt_alarm = GUI_TextSelectHelper(Transmitter.batt_alarm, 
            MIN_BATTERY_ALARM, MAX_BATTERY_ALARM, dir, MIN_BATTERY_ALARM_STEP, 500, &changed);
    sprintf(tempstring, "%2d.%02dV", Transmitter.batt_alarm / 1000, (Transmitter.batt_alarm % 1000) / 10);
    return tempstring;
}

static const char *batalarmwarn_select_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    u8 changed;
    u16 batt_warning = Transmitter.batt_warning_interval;
    batt_warning = GUI_TextSelectHelper(batt_warning,
            MIN_BATTERY_WARNING_INTERVAL, MAX_BATTERY_WARNING_INTERVAL, dir, 5, 30, &changed);
    if (changed)
        Transmitter.batt_warning_interval = batt_warning;
    if( 0 == batt_warning)
	return _tr("Off");
    TIMER_SetString(tempstring, Transmitter.batt_warning_interval * 1000);
    return tempstring;
}

static void press_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    cp->enable = (long)data;
#if HAS_RTC
    if (cp->enable == SET_CLOCK) {
        PAGE_PushByID(PAGEID_RTC, 0);
        return;
    }
#endif
#if HAS_TOUCH
    if (cp->enable == CALIB_TOUCH)
        PAGE_PushByID(PAGEID_TOUCH, 0);
    else if (cp->enable == CALIB_STICK)
#endif
    {
        calibrate_state = CALI_CENTER; // bug fix: must reset state before calibrating
        PAGE_PushByID(PAGEID_CALIB, 0);
    }
}

#ifndef NO_LANGUAGE_SUPPORT
static const char *langstr_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    return _tr("Change");
}

static void lang_select_cb(guiObject_t *obj, const void *data)
{
    (void)data;
    (void)obj;
    PAGE_PushByID(PAGEID_LANGUAGE, 0);
}
#endif

static const char *units_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 changed;
    u8 mask = data ? TELEMUNIT_FAREN : TELEMUNIT_FEET;
    u8 type = (Transmitter.telem & mask) ? 1 : 0;
    type = GUI_TextSelectHelper(type, 0, 1, dir, 1, 1, &changed);
    if (changed) {
        if (type) {
            Transmitter.telem |= mask;
        } else {
            Transmitter.telem &= ~mask;
        }
    }
    if (data) {
        return type ? _tr("Fahren") : _tr("Celsius");
    } else {
        return type ? _tr("Foot") : _tr("Meter");
    }
}

static const char *_music_shutdown_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    (void)obj;
    Transmitter.music_shutdown = GUI_TextSelectHelper(Transmitter.music_shutdown, 0, 1, dir, 1, 1, NULL);
    if (Transmitter.music_shutdown == 0)
        return _tr("Off");
    else
        return _tr("On");
}

static const char *_buzz_vol_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    u8 *unsigned_data = (u8 *)data;
    u8 changed;
    if (GUI_IsTextSelectEnabled(obj)) {
        *unsigned_data = GUI_TextSelectHelper(*unsigned_data, 0, 10, dir, 1, 1, &changed);
        if (changed)
            MUSIC_Play(MUSIC_VOLUME);
    }
    if (*unsigned_data == 0)
        return _tr("Off");
    sprintf(tempstring, "%d", *unsigned_data);
    return tempstring;
}
