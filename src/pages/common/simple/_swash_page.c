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

static struct model_page * const mp = &pagemem.u.model_page;
guiObject_t *itemObj[3];
static s8 swashmix[3];

static void update_swashmixes()
{
    u8 state = 1;
    if (Model.swash_type ==  SWASH_TYPE_NONE)
        state = 0;
    for (u8 i = 0; i < 3; i++) {
        GUI_TextSelectEnable(itemObj[i], state);
    }
}

static const char *swash_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    (void)data;
    u8 changed = 1;
    Model.swash_type = GUI_TextSelectHelper(Model.swash_type, SWASH_TYPE_NONE , SWASH_TYPE_90, dir, 1, 1, &changed);
    if (changed)
        update_swashmixes();
    switch(Model.swash_type) {
        case SWASH_TYPE_NONE:
            sprintf(mp->tmpstr,"1%s", _tr("Servo"));
            break;
        case SWASH_TYPE_120:
            sprintf(mp->tmpstr,"3%s 120", _tr("Servo"));
            break;
        case SWASH_TYPE_120X:
            sprintf(mp->tmpstr,"3%s 120x", _tr("Servo"));
            break;
        case SWASH_TYPE_140:
            sprintf(mp->tmpstr,"3%s 140", _tr("Servo"));
            break;
        case SWASH_TYPE_90:
            sprintf(mp->tmpstr,"3%s 90", _tr("Servo"));
            break;
        default:
            break;
    }
    return mp->tmpstr;
}

static const char *swashmix_val_cb(guiObject_t *obj, int dir, void *data)
{
    (void)data;
    if (!GUI_IsTextSelectEnabled(obj))
        return _tr("None");
    int i = (long)data;
    u8 changed = 1;
    swashmix[i] = GUI_TextSelectHelper(swashmix[i], -100, 100, dir, 1, 5, &changed);
    if (changed) {
        u8 mask = SWASH_INV_ELEVATOR_MASK;
        switch (i) {
        case 0: // aile
            mask = SWASH_INV_AILERON_MASK;
            break;
        case 1:  // elev
            mask = SWASH_INV_ELEVATOR_MASK;
            break;
        default:  // pit
            mask = SWASH_INV_COLLECTIVE_MASK;
            break;
        }
        if (swashmix[i] >= 0) {
            Model.swashmix[i] = swashmix[i];
            Model.swash_invert = Model.swash_invert & ~mask;
        } else  {
            Model.swashmix[i] = -swashmix[i];
            Model.swash_invert = Model.swash_invert | mask;
        }
    }
    sprintf(mp->tmpstr, "%d", swashmix[i]);
    return mp->tmpstr;
}

static void get_swash()
{
    for (u8 i = 0; i < 3; i++) {
        u8 mask = SWASH_INV_ELEVATOR_MASK;
        switch (i) {
        case 0: // aile
            mask = SWASH_INV_AILERON_MASK;
            break;
        case 1:  // elev
            mask = SWASH_INV_ELEVATOR_MASK;
            break;
        default:  // pit
            mask = SWASH_INV_COLLECTIVE_MASK;
            break;
        }
        if (Model.swash_invert & mask)
            swashmix[i] = -Model.swashmix[i];
        else
            swashmix[i] = Model.swashmix[i];
    }
}

