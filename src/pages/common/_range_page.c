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

static struct range_page * const mp = &pagemem.u.range_page;

static void _draw_page(int);

void RANGE_test(int start) {
  if (start != mp->testing) {
      if (start) {
          mp->old_power = Model.tx_power;
          Model.tx_power = TXPOWER_100uW;
      } else {
          Model.tx_power = mp->old_power;
      }
      mp->testing = start;
      _draw_page(1);
  }
}

void PAGE_RangeInit(int page) {
    (void)page;
    PAGE_SetModal(0);
    memset(mp, 0, sizeof(&mp));
    mp->old_power = Model.tx_power;
    _draw_page(PROTOCOL_HasPowerAmp(Model.protocol) 
               && Model.tx_power != TXPOWER_100uW);
}

void PAGE_RangeExit() {
    RANGE_test(0);
}

    
