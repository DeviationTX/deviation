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

#include "petit_io.h"
#include "target.h"

//SECTOR_OFFSET is also defined in target/devo8/msc2/mass_mal.c
//Make sure they both match!
#define SECTOR_OFFSET 54


/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (void)
{
	DSTATUS stat = 0;
	// No initialization needed
	return stat;
}



/*-----------------------------------------------------------------------*/
/* Read Partial Sector                                                   */
/*-----------------------------------------------------------------------*/

DRESULT disk_readp (
	BYTE* dest,			/* Pointer to the destination object */
	DWORD sector,		/* Sector number (LBA) */
	WORD sofs,			/* Offset in the sector */
	WORD count			/* Byte count (bit15:destination) */
)
{
	SPIFlash_ReadBytes((sector + SECTOR_OFFSET) * 0x1000 + sofs, count, dest);

	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Partial Sector                                                  */
/*-----------------------------------------------------------------------*/

DRESULT disk_writep_rand (
	const BYTE* src,		/* Pointer to the destination object */
	DWORD sector,			/* Sector number (LBA) */
	WORD sofs,			/* Offset in the sector */
	WORD count			/* Byte count (bit15:destination) */
)
{
	SPIFlash_WriteBytes((sector + SECTOR_OFFSET) * 0x1000 + sofs, count, src);
	return RES_OK;
}

DRESULT disk_writep (
	const BYTE* buff,		/* Pointer to the data to be written, NULL:Initiate/Finalize write operation */
	DWORD sc		/* Sector number (LBA) or Number of bytes to send */
)
{
	static DWORD pos;

	if (!buff) {
		if (sc) {
			// Initiate write process
			pos = (sc + SECTOR_OFFSET) * 0x1000;
		} else {
			// Finalize write process
			pos = 0;
		}
	} else {
		// Send data to the disk
		SPIFlash_WriteBytes(pos, sc, buff);
		pos += sc;
	}
	return RES_OK;
}

