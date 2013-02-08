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

#define gui (&gui_objs.u.stddrexp)
static struct mixer_page * const mp = &pagemem.u.mixer_page;
typedef enum {
    DREXP_AIL = 0,
    DREXP_ELE,
    DREXP_RUD,
} DREXPType;
static DREXPType drexp_type = DREXP_AIL;
static u8 current_pit_mode = 0;
#define MAX_SCALAR 125

static void _refresh_page();

static void get_mixers()
{
    if (drexp_type == DREXP_AIL) {
        SIMPLEMIX_GetMixers(mp->mixer_ptr, mapped_simple_channels.aile, DREXPMIXER_COUNT);
    } else if (drexp_type == DREXP_ELE) {
        SIMPLEMIX_GetMixers(mp->mixer_ptr, mapped_simple_channels.elev, DREXPMIXER_COUNT);
    } else {
        SIMPLEMIX_GetMixers(mp->mixer_ptr, mapped_simple_channels.rudd, DREXPMIXER_COUNT);
    }
}

static const char *set_type_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    u8 changed;
    drexp_type = GUI_TextSelectHelper(drexp_type, DREXP_AIL, DREXP_RUD, dir, 1, 1, &changed);
    if (changed) {
        get_mixers();
        _refresh_page();
    }
    switch (drexp_type) {
    case DREXP_AIL:
        strcpy(mp->tmpstr, (const char *)_tr("AIL"));
        break;
    case DREXP_ELE:
        strcpy(mp->tmpstr, (const char *)_tr("ELE"));
        break;
    default:
        strcpy(mp->tmpstr, (const char *)_tr("RUD"));
        break;
    }
    return mp->tmpstr;
}

static const char *set_dr_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 pit_mode = (long)data;
    u8 changed = 1;
    mp->mixer_ptr[pit_mode]->scalar = GUI_TextSelectHelper(mp->mixer_ptr[pit_mode]->scalar,
            0, MAX_SCALAR, dir, 1, LONG_PRESS_STEP, &changed);
    if (changed || (GUI_GetSelected() == obj && current_pit_mode != pit_mode)) {
        current_pit_mode = pit_mode;
        GUI_Redraw(&gui->graph);;
    }
    sprintf(mp->tmpstr, "%d", mp->mixer_ptr[pit_mode]->scalar);
    strcat(mp->tmpstr, "%");
    return mp->tmpstr;
}

const char *drexplabel_cb(guiObject_t *obj, const void *data)
{
    (void)obj;
    u8 i = (long)data;
    sprintf(mp->tmpstr, _tr("Position %d"), i);
    return mp->tmpstr;
}

static const char *set_exp_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    u8 pit_mode = (long)data;
    u8 changed = 1;
    struct Curve *curve = &(mp->mixer_ptr[pit_mode]->curve);
    curve->points[0] = GUI_TextSelectHelper(curve->points[0], -100, 100, dir, 1, LONG_PRESS_STEP, &changed);
    if (changed || (GUI_GetSelected() == obj && current_pit_mode != pit_mode))  {
        curve->points[1] = curve->points[0];
        current_pit_mode = pit_mode;
        GUI_Redraw(&gui->graph);
    }
    if (curve->points[0] == 0)
        strcpy(mp->tmpstr, _tr("LIN"));
    else {
        sprintf(mp->tmpstr, "%d", curve->points[0]);
        strcat(mp->tmpstr, "%");
    }
    return mp->tmpstr;
}

static u8 curpos_cb(s16 *x, s16 *y, u8 pos, void *data)
{
    (void)data;
    if (pos != 0)
        return 0;
    *x = mp->raw[MIXER_SRC(mp->mixer_ptr[current_pit_mode]->src)];
    if (*x > CHAN_MAX_VALUE)
        *x = CHAN_MAX_VALUE;
    else if (*x  < CHAN_MIN_VALUE)
        *x = CHAN_MIN_VALUE;
    s16 ymax = CHAN_MAX_VALUE/100 * MAX_SCALAR;
    s16 ymin = -ymax;
    *y = SIMPLEMIX_EvalMixerCb(*x, mp->mixer_ptr[current_pit_mode], ymax, ymin);
    return 1;
}

static s16 show_curve_cb(s16 xval, void *data)
{
    (void)data;
    s16 yval = CURVE_Evaluate(xval, &(mp->mixer_ptr[current_pit_mode]->curve));
    yval = yval * mp->mixer_ptr[current_pit_mode]->scalar / 100 ;
    return yval;
}

