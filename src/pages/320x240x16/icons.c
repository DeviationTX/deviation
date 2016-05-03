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
#include "gui/gui.h"
#include "icons.h"

const struct ImageMap icons[] = {
    [ICON_EXIT]       = {"media/exit"     IMG_EXT, 32, 31, 0, 0},
    [ICON_OPTIONS]    = {"media/options"  IMG_EXT, 32, 31, 0, 0},
    [ICON_PREVPAGE]   = {"media/prevpage" IMG_EXT, 32, 31, 0, 0},
    [ICON_NEXTPAGE]   = {"media/nextpage" IMG_EXT, 32, 31, 0, 0},
    [ICON_CHANTEST]   = {"media/chansico" IMG_EXT, 32, 31, 0, 0},
    [ICON_MODELICO]   = {"media/modelico" IMG_EXT, 32, 31, 0, 0},
    [ICON_ORDER]      = {"media/orderico" IMG_EXT, 32, 31, 0, 0},
    [ICON_LAYOUT_ADD] = {"media/lay_add"  IMG_EXT, 32, 31, 0, 0},
    [ICON_LAYOUT_CFG] = {"media/lay_cfg"  IMG_EXT, 32, 31, 0, 0},
};
