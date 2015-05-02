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
#include "common.h"

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/
static const struct {
	void (*ReadBytes)(u32 readAddress, u32 length, u8 * buffer);
	int (*ReadBytesStopCR)(u32 readAddress, u32 length, u8 * buffer);
	void (*WriteBytes)(u32 writeAddress, u32 length, const u8 * buffer);
	void (*EraseSector)(u32 sectorAddress);
	long SECTOR_OFFSET;
} drive[] = {
	{SPIFlash_ReadBytes, SPIFlash_ReadBytesStopCR, SPIFlash_WriteBytes, SPIFlash_EraseSector, SPIFLASH_SECTOR_OFFSET},
#ifdef MEDIA_DRIVE
	{ MEDIA_DRIVE },
#endif
};

u8 _drive_num = 0;
u8 _stop_on_cr = 0;

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
	//printf("Reading sector: %d, offset: %d size: %d\n", (int)sector, (int)sofs, (int)count);
	drive[_drive_num].ReadBytes((sector + drive[_drive_num].SECTOR_OFFSET) * 0x1000 + sofs, count, dest);
	//int max = count > 64 ? 64 : count;
	//for(int i = 0; i < max; i++) {
	//    printf("%02x ", dest[i]);
	//}
	//printf("\n");

	return RES_OK;
}

DRESULT disk_readp_cnt (
	BYTE* dest,			/* Pointer to the destination object */
	DWORD sector,		/* Sector number (LBA) */
	WORD sofs,			/* Offset in the sector */
	WORD count,			/* Byte count (bit15:destination) */
	WORD *actual			/* Actual Byte count read (bit15:destination) */
)
{
	if (_stop_on_cr) {
                *actual = drive[_drive_num].ReadBytesStopCR((sector + drive[_drive_num].SECTOR_OFFSET) * 0x1000 + sofs, count, dest);
	} else {
		*actual = count;
                drive[_drive_num].ReadBytes((sector + drive[_drive_num].SECTOR_OFFSET) * 0x1000 + sofs, count, dest);
	}
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
	drive[_drive_num].WriteBytes((sector + drive[_drive_num].SECTOR_OFFSET) * 0x1000 + sofs, count, src);
	return RES_OK;
}

DRESULT disk_erasep (
        DWORD sc
)
{
	drive[_drive_num].EraseSector(pos + drive[_drive_num].SECTOR_OFFSET) * 0x1000);
{

DRESULT disk_writep (
	const BYTE* buff,		/* Pointer to the data to be written, NULL:Initiate/Finalize write operation */
	DWORD sc		/* Sector number (LBA) or Number of bytes to send */
)
{
	static DWORD pos;

	if (!buff) {
		if (sc) {
			// Initiate write process
			pos = (sc + drive[_drive_num].SECTOR_OFFSET) * 0x1000;
                        drive[_drive_num].EraseSector(pos);
		} else {
			// Finalize write process
			pos = 0;
		}
	} else {
		// Send data to the disk
                drive[_drive_num].WriteBytes(pos, sc, buff);
		pos += sc;
	}
	return RES_OK;
}

