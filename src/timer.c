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
#include "timer.h"
#include "music.h"
#include "config/model.h"
#include "config/tx.h"

static u8 timer_state[NUM_TIMERS];
static s32 timer_val[NUM_TIMERS];
static s32 last_time[NUM_TIMERS];

void TIMER_SetString(char *str, s32 time)
{
    //u8 h = time / 3600;
    //u8 m = (time - h*3600) / 60;
    //u8 s = time -h*3600 - m*60;
    //sprintf(str, "%02d:%02d:%02d", h, m, s);
    char neg;
    if (time < 0) {
        neg = 1;
        time = -time;
    } else {
        neg = 0;
    }
    time = time / 1000; //Convert to seconds
    u8 h = time / 3600;
    u8 m = (time - h*3600) / 60;
    u8 s = time -h*3600 - m*60;
    if( h < 1)
    	sprintf(str, "%s%02d:%02d", neg ? "-" : "", m, s);
    else
	sprintf(str, "%s%02d:%02d:%02d", neg ? "-" : "", h, m, s);
}

const char *TIMER_Name(char *str, u8 timer)
{
    sprintf(str, "%s%d", _tr("Timer"), timer+1);
    return str;
}

void TIMER_StartStop(u8 timer)
{
    timer_state[timer] ^= 1;
    if(timer_state[timer]) {
        last_time[timer] = CLOCK_getms();
    }
}

void TIMER_Reset(u8 timer)
{
    timer_state[timer] = 0;
    if (Model.timer[timer].type == TIMER_STOPWATCH) {
        timer_val[timer] = 0;
    } else if(Model.timer[timer].type == TIMER_COUNTDOWN) {
        timer_val[timer] = Model.timer[timer].timer * 1000;
    } else if(Model.timer[timer].type == TIMER_PERMANENT ) {
	timer_val[timer] = Model.timer[timer].val;
    }
}

s32 TIMER_GetValue(u8 timer)
{
    return timer_val[timer];
}

void TIMER_Init()
{
    u8 i;
    for (i = 0; i < NUM_TIMERS; i++)
	TIMER_Reset(i);
}

void TIMER_Update()
{
    u8 i;
    u32 t = CLOCK_getms();
    for (i = 0; i < NUM_TIMERS; i++) {
        if (Model.timer[i].src) {
            s16 val;
            if (MIXER_SRC(Model.timer[i].src) <= NUM_INPUTS) {
                volatile s16 *raw = MIXER_GetInputs();
                val = raw[MIXER_SRC(Model.timer[i].src)];
            } else {
                val = MIXER_GetChannel(Model.timer[i].src - NUM_INPUTS - 1, APPLY_SAFETY);
            }
            if (MIXER_SRC_IS_INV(Model.timer[i].src))
                val = -val;
            u8 new_state = (val - CHAN_MIN_VALUE > (CHAN_MAX_VALUE - CHAN_MIN_VALUE) / 20) ? 1 : 0;
            if (new_state != timer_state[i]) {
                if (new_state)
                    last_time[i] = t;
                timer_state[i] = new_state;
            }
        }
        if (timer_state[i]) {
            s32 delta = t - last_time[i];
	   if (Model.timer[i].type == TIMER_PERMANENT) {
		timer_val[i] += delta;
		if( timer_val[i] >= 359999900) // Reset when 99h59mn59sec
		    timer_val[i] = 0 ;
		Model.timer[i].val = timer_val[i];
            } else if (Model.timer[i].type == TIMER_STOPWATCH) {
                timer_val[i] += delta;
            } else {
                s32 warn_time;
                // start to beep  for each prealert_interval at the last prealert_time(seconds)
                if (Transmitter.countdown_timer_settings.prealert_time != 0 &&
                    Transmitter.countdown_timer_settings.prealert_interval != 0 &&
                    timer_val[i] > Transmitter.countdown_timer_settings.prealert_interval &&
                    timer_val[i] < (s32)Transmitter.countdown_timer_settings.prealert_time + 1000) { // give extra 1seconds
                    warn_time = ((timer_val[i] / Transmitter.countdown_timer_settings.prealert_interval)
                            * Transmitter.countdown_timer_settings.prealert_interval);
                    if (timer_val[i] > warn_time && (timer_val[i] - delta) <= warn_time) {
                        MUSIC_Play(MUSIC_TIMER_WARNING);
                    }
                }
                // Beep once for each timeup_interval past 0
                if (timer_val[i] < 0 && Transmitter.countdown_timer_settings.timeup_interval != 0) {
                    warn_time = ((timer_val[i] - Transmitter.countdown_timer_settings.timeup_interval) / Transmitter.countdown_timer_settings.timeup_interval)
                            * Transmitter.countdown_timer_settings.timeup_interval;
                    if (timer_val[i] > warn_time && (timer_val[i] - delta) <= warn_time) {
                        MUSIC_Play(MUSIC_ALARM1 + i);
                    }
                }
                if (timer_val[i] >= 0 && timer_val[i] < delta) {
                    MUSIC_Play(MUSIC_ALARM1 + i);
                }
                timer_val[i] -= delta;
            }
            last_time[i] = t;
	} 
	if (Model.timer[i].resetsrc) {
            s16 val;
            if (MIXER_SRC(Model.timer[i].resetsrc) <= NUM_INPUTS) {
                volatile s16 *raw = MIXER_GetInputs();
                val = raw[MIXER_SRC(Model.timer[i].resetsrc)];
            } else {
                val = MIXER_GetChannel(Model.timer[i].resetsrc - NUM_INPUTS - 1, APPLY_SAFETY);
            }
            if (MIXER_SRC_IS_INV(Model.timer[i].resetsrc))
                val = -val;
            if (val - CHAN_MIN_VALUE > (CHAN_MAX_VALUE - CHAN_MIN_VALUE) / 20) {
		TIMER_Reset(i);
	    }
        }
    }
}
