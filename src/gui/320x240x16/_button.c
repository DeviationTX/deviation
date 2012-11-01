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

const struct ImageMap *_button_image_map(enum ButtonType type)
{
    switch (type) {
        case BUTTON_96:    return &image_map[FILE_BTN96_24]; break;
        case BUTTON_48:    return &image_map[FILE_BTN48_24]; break;
        case BUTTON_96x16: return &image_map[FILE_BTN96_16]; break;
        case BUTTON_64x16: return &image_map[FILE_BTN64_16]; break;
        case BUTTON_48x16: return &image_map[FILE_BTN48_16]; break;
        case BUTTON_32x16: return &image_map[FILE_BTN32_16]; break;
        default: return NULL;
    }
    return NULL;
}
