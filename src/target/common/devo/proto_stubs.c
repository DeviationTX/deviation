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

/*The following will force the loading of various
  functions used in the protocol modules, but unused elsewhere
  in Deviation.
  Note that we lie aboiut the arguments to these functions. It is
  Important that the actual functions never execute
*/
extern void CLOCK_StartTimer();
extern void spi_disable();
extern void spi_set_bidirectional_receive_only_mode();
extern void spi_read();
extern void spi_set_unidirectional_mode();
extern void rcc_peripheral_disable_clock();
extern void _usleep();
extern void TELEMETRY_SetUpdated();
extern void TELEMETRY_SetType();
extern void MCU_SerialNumber();
extern void PROTO_CS_HI();
extern void PROTO_CS_LO();
extern void MUSIC_Beep();
extern void rand32();

void PROTO_Stubs(int idx)
{
    if (! idx)
        return;
    CLOCK_StartTimer();

    spi_disable();
    spi_set_bidirectional_receive_only_mode();
    spi_read();
    spi_set_unidirectional_mode();
    TELEMETRY_SetUpdated();
    TELEMETRY_SetType();
    rcc_peripheral_disable_clock();
    _usleep();
    MCU_SerialNumber();
    PROTO_CS_HI();
    PROTO_CS_LO();
    MUSIC_Beep();
    rand32();
}
