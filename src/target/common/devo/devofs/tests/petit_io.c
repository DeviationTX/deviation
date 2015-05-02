/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for Petit FatFs (C)ChaN, 2009      */
/*-----------------------------------------------------------------------*/
/*
/ Petit FatFs module is an open source software to implement FAT file system to
/ small embedded systems. This is a free software and is opened for education,
/ research and commercial developments under license policy of following trems.
/
/  Copyright (C) 2010, ChaN, all right reserved.
/
/ * The Petit FatFs module is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial use UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
*/

#include "petit_io.h"
#include <stdio.h>
#include <string.h>

extern char image_file[1024];

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/
static FILE *fh;
DSTATUS disk_initialize (void)
{
	DSTATUS stat = 0;
	fh = fopen(image_file, "r+");
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
	printf("Reading sector: %d, offset: %d size: %d\n", (int)sector, (int)sofs, (int)count);
	fseek(fh, sector * 4096 + sofs, SEEK_SET);
	fread(dest, count, 1, fh);
        int max = count > 64 ? 64 : count;
        int i;
        for(i = 0; i < max; i++) {
            printf("%02x ", dest[i]);
        }
        printf("\n");

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
	printf("Writing sector: %d, offset: %d size: %d\n", (int)sector, (int)sofs, (int)count);
	fseek(fh, sector * 4096 + sofs, SEEK_SET);
	fwrite(src, count, 1, fh);
	return RES_OK;
}

DRESULT disk_erasep (
        DWORD sc
)
{
	unsigned char data[4096];
	memset(data, 0, 4096);
	// Initiate write process
	fseek(fh, sc * 4096, SEEK_SET);
	fwrite(data, 4096, 1, fh);
}
DRESULT disk_writep (
	const BYTE* buff,		/* Pointer to the data to be written, NULL:Initiate/Finalize write operation */
	DWORD sc		/* Sector number (LBA) or Number of bytes to send */
)
{
	DRESULT res;


	if (!buff) {
		if (sc) {
                        unsigned char data[4096];
                        memset(data, 0, 4096);
			// Initiate write process
			fseek(fh, sc * 4096, SEEK_SET);
			fwrite(data, 4096, 1, fh);
			fseek(fh, sc * 4096, SEEK_SET);
		} else {
			// Finalize write process
		}
	} else {
		// Send data to the disk
		fwrite(buff, sc, 1, fh);
	}

	return res;
}

