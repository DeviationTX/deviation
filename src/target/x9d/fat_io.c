/*-----------------------------------------------------------------------*/
/* mmc/sdsc/sdhc (in spi mode) control module for stm32 version 1.1.6    */
/* (c) martin thomas, 2010 - based on the avr mmc module (c)chan, 2007   */
/*                                                                       */
/* this is the libopencm3 version                                        */
/*-----------------------------------------------------------------------*/
/* This file is a combination of the taranis 'diskio.c' from opentx and
 * Martin Thomas's port of the STM32 SD SPI code to libopencm3
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <string.h>
#include <stdio.h>
#include "FatFs/diskio.h"
#include "FatFs/ff.h"
#include "x9d_ports.h"

#include <libopencm3/stm32/f2/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/dma.h>

/* Definitions for MMC/SDC command */
#define CMD0    (0x40+0)        /* GO_IDLE_STATE */
#define CMD1    (0x40+1)        /* SEND_OP_COND (MMC) */
#define ACMD41  (0xC0+41)       /* SEND_OP_COND (SDC) */
#define CMD8    (0x40+8)        /* SEND_IF_COND */
#define CMD9    (0x40+9)        /* SEND_CSD */
#define CMD10   (0x40+10)       /* SEND_CID */
#define CMD12   (0x40+12)       /* STOP_TRANSMISSION */
#define ACMD13  (0xC0+13)       /* SD_STATUS (SDC) */
#define CMD16   (0x40+16)       /* SET_BLOCKLEN */
#define CMD17   (0x40+17)       /* READ_SINGLE_BLOCK */
#define CMD18   (0x40+18)       /* READ_MULTIPLE_BLOCK */
#define CMD23   (0x40+23)       /* SET_BLOCK_COUNT (MMC) */
#define ACMD23  (0xC0+23)       /* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24   (0x40+24)       /* WRITE_BLOCK */
#define CMD25   (0x40+25)       /* WRITE_MULTIPLE_BLOCK */
#define CMD55   (0x40+55)       /* APP_CMD */
#define CMD58   (0x40+58)       /* READ_OCR */

/* Card-Select Controls  (Platform dependent) */
#define SELECT()        gpio_clear(GPIO_PORT_CS, GPIOCS)    /* MMC CS = L */
#define DESELECT()      gpio_set(GPIO_PORT_CS, GPIOCS)      /* MMC CS = H */

#if (_MAX_SS != 512) || (_FS_READONLY == 0) || (STM32_SD_DISK_IOCTRL_FORCE == 1)
#define STM32_SD_DISK_IOCTRL   1
#else
#define STM32_SD_DISK_IOCTRL   0
#endif

#define BOOL   bool
#define FALSE  false
#define TRUE   true

/* Card type flags (CardType) */
#define CT_MMC              0x01
#define CT_SD1              0x02
#define CT_SD2              0x04
#define CT_SDC              (CT_SD1|CT_SD2)
#define CT_BLOCK            0x08

/*uint32_t Card_ID[4] ;
uint32_t Card_SCR[2] ;
uint32_t Card_CSD[4] ;
int32_t Card_state; //  = SD_ST_STARTUP ;
volatile uint32_t Card_initialized = 0;
uint32_t Sd_rca ;
uint32_t Cmd_A41_resp ;
uint8_t  cardType;
uint32_t transSpeed; */

static const DWORD socket_state_mask_cp = (1 << 0);
static const DWORD socket_state_mask_wp = (1 << 1);

static volatile
DSTATUS Stat = STA_NOINIT;      /* Disk status */

static volatile
DWORD Timer1, Timer2;   /* 100Hz decrement timers */

static
BYTE CardType;                  /* Card type flags */

/*---------------------------------------------------------------------------*/
/** @brief Set the SPI Interface Speed

Sets the SPI baudrate prescaler value to either divide by 4 or divide by 256.

@param[in] speed_setting: INTERFACE_SLOW, INTERFACE_FAST

*/
enum speed_setting { INTERFACE_SLOW, INTERFACE_FAST };

static void interface_speed( enum speed_setting speed )
{
	if ( speed == INTERFACE_SLOW )
    {
		/* Set slow clock (100k-400k) */
		spi_set_baudrate_prescaler(SPI_SD,SPI_BaudRatePrescaler_slow);
	}
    else
    {
		/* Set fast clock (depends on the CSD) */
		spi_set_baudrate_prescaler(SPI_SD,SPI_BaudRatePrescaler_fast);
	}
}

/*---------------------------------------------------------------------------*/
/** @brief Initialise GPIO to Read Write Protect Pin

*/
static void socket_wp_init(void)
{
        return;
}

/*---------------------------------------------------------------------------*/
/** @brief Read the Write Protect Pin

Socket's Write-Protection Pin: high = write-protected, low = writable

@returns 16 bit mask for the socket state if protected, or zero if writeable. 
*/
static inline DWORD socket_is_write_protected(void)
{
        return 0; /* fake not protected */
}

/*---------------------------------------------------------------------------*/
/** @brief Initialise the Card Present Pin

*/
static void socket_cp_init(void)
{
    rcc_peripheral_enable_clock(&RCC_GPIO, RCC_GPIO_CP);
    gpio_mode_setup(GPIO_PORT_CP, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, GPIOCP);
}

/*---------------------------------------------------------------------------*/
/** @brief Read the Card Present Pin

Socket's Card-Present Pin: high = socket empty, low = card inserted

@returns 16 bit mask for the socket state if empty, or zero if inserted. 
*/
static inline DWORD socket_is_empty(void)
{
	return gpio_get(GPIO_PORT_CP,GPIOCP) ? socket_state_mask_cp : 0;
}

/*---------------------------------------------------------------------------*/
/** @brief Switch on the Socket Power

The power control pin is set low to turn on (high side PMOS).
Internal pullup is used but shouldn't be needed as circuit should have this.

@param[in] on: Boolean.
*/
static void card_power(BYTE on)
{
        on=on;
}

/*---------------------------------------------------------------------------*/
/** @brief Check if Socket Power is on

The power control pin is set low to turn on.

@returns int 1 (TRUE) if the power is on.
*/
#if (STM32_SD_DISK_IOCTRL == 1)
static int chk_power(void)
{
        return 1; /* fake powered */
}
#endif

/*---------------------------------------------------------------------------*/
/** @brief Transmit/Receive a byte to MMC via SPI

@param[in] out: BYTE value to send.
*/
static BYTE stm32_spi_rw( BYTE out )
{
    return spi_xfer(SPI_SD,out);
}


/*-----------------------------------------------------------------------*/
/* Transmit a byte to MMC via SPI  (Platform dependent)                  */
/*-----------------------------------------------------------------------*/

#define xmit_spi(dat)  stm32_spi_rw(dat)

/*---------------------------------------------------------------------------*/
/** @brief Receive a byte from MMC via SPI

Sends a byte 0xFF and picks up whatever returns.

@returns BYTE value received.
*/
static BYTE rcvr_spi (void)
{
        return stm32_spi_rw(0xff);
}

/* Alternative macro to receive data fast */
#define rcvr_spi_m(dst)  *(dst)=stm32_spi_rw(0xff)



/*---------------------------------------------------------------------------*/
/** @brief Wait for the Card to become Ready

@returns BYTE value of a status result. 0xFF means success, otherwise timeout.
*/
static BYTE wait_ready (void)
{
        BYTE res;


        Timer2 = 50;    /* Wait for ready in timeout of 500ms */
        rcvr_spi();
        do
                res = rcvr_spi();
        while ((res != 0xFF) && Timer2);

        return res;
}



/*---------------------------------------------------------------------------*/
/** @brief Deselect the card and release SPI bus

*/
static void release_spi (void)
{
        DESELECT();
        rcvr_spi();
}

#ifdef STM32_SD_USE_DMA
/** @brief Transmit/Receive Block using DMA

@param[in] receive: Boolean false for sending to SPI, true for reading
@param[in] *buff: Pointer to buffer of BYTE
@param[in] btr: UINT byte count (multiple of 2 for send, 512 always for receive)
*/
static void stm32_dma_transfer(
	BOOL receive,		/* FALSE for buff->SPI, TRUE for SPI->buff               */
	const BYTE *buff,	/* receive TRUE  : 512 byte data block to be transmitted
						   receive FALSE : Data buffer to store received data    */
	UINT btr 			/* receive TRUE  : Byte count (must be multiple of 2)
						   receive FALSE : Byte count (must be 512)              */
)
{
	WORD rw_workbyte[] = { 0xffff };

	/* Enable DMA1 Clock */
	rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_DMA1EN);

        /* Reset DMA channels*/
	dma_stream_reset(DMA1, DMA_STREAM_SPI_SD_RX);
	dma_stream_reset(DMA1, DMA_STREAM_SPI_SD_TX);
	
	/* DMA1 read channel2 configuration SPI1 RX ---------------------------------------------*/
	dma_channel_select(DMA1, DMA_STREAM_SPI_SD_RX, DMA_Channel_SPI_RX);
	dma_set_peripheral_address(DMA1, DMA_STREAM_SPI_SD_RX, (DWORD) &SPI_DR(SPI_SD));
	dma_set_peripheral_size(DMA1, DMA_STREAM_SPI_SD_RX, DMA_SxCR_PSIZE_8BIT);
	dma_set_memory_size(DMA1, DMA_STREAM_SPI_SD_RX, DMA_SxCR_MSIZE_8BIT);
	dma_disable_peripheral_increment_mode(DMA1, DMA_STREAM_SPI_SD_RX);
	dma_set_number_of_data(DMA1, DMA_STREAM_SPI_SD_RX,btr);
	dma_set_priority(DMA1, DMA_STREAM_SPI_SD_RX, DMA_SxCR_PL_VERY_HIGH);
	dma_enable_fifo_mode(DMA1, DMA_STREAM_SPI_SD_RX);
	dma_set_fifo_threshold(DMA1,DMA_STREAM_SPI_SD_RX, DMA_SxFCR_FTH_4_4_FULL);

	/* DMA1 write channel3 configuration SPI1 TX ---------------------------------------------*/
	dma_channel_select(DMA1, DMA_STREAM_SPI_SD_TX, DMA_Channel_SPI_TX);
	dma_set_peripheral_address(DMA1, DMA_STREAM_SPI_SD_TX, (DWORD) &SPI_DR(SPI_SD));
	dma_set_peripheral_size(DMA1, DMA_STREAM_SPI_SD_TX, DMA_SxCR_PSIZE_8BIT);
	dma_set_memory_size(DMA1, DMA_STREAM_SPI_SD_TX, DMA_SxCR_MSIZE_8BIT);
	dma_disable_peripheral_increment_mode(DMA1, DMA_STREAM_SPI_SD_TX);
	dma_set_number_of_data(DMA1, DMA_STREAM_SPI_SD_TX, btr);
	dma_set_priority(DMA1, DMA_STREAM_SPI_SD_TX, DMA_SxCR_PL_VERY_HIGH);
	dma_enable_fifo_mode(DMA1, DMA_STREAM_SPI_SD_TX);
	dma_set_fifo_threshold(DMA1,DMA_STREAM_SPI_SD_TX, DMA_SxFCR_FTH_4_4_FULL);

	//seperate RX & TX
	if ( receive ) { //true =read 

		/* DMA1 read channel configuration SPI1 RX ---------------------------------------------*/
		dma_set_memory_address(DMA1,DMA_STREAM_SPI_SD_RX,(DWORD)buff);
		dma_set_transfer_mode(DMA1, DMA_STREAM_SPI_SD_RX, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
		dma_enable_memory_increment_mode(DMA1,DMA_STREAM_SPI_SD_RX);

		/* DMA1 write channel configuration SPI1 TX ---------------------------------------------*/
		dma_set_memory_address(DMA1,DMA_STREAM_SPI_SD_TX,(DWORD)rw_workbyte);
		dma_set_transfer_mode(DMA1, DMA_STREAM_SPI_SD_TX, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
		dma_disable_memory_increment_mode(DMA1,DMA_STREAM_SPI_SD_TX);

	} else {//false = write

#if _FS_READONLY == 0 //READ AND WRITE = write enabled.
		/* DMA1 read channel configuration SPI1 RX ---------------------------------------------*/
		dma_set_memory_address(DMA1,DMA_STREAM_SPI_SD_RX,(DWORD)rw_workbyte);
		dma_set_transfer_mode(DMA1, DMA_STREAM_SPI_SD_RX, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
		dma_disable_memory_increment_mode(DMA1,DMA_STREAM_SPI_SD_RX);

		/* DMA1 write channel configuration SPI1 TX ---------------------------------------------*/
		dma_set_memory_address(DMA1,DMA_STREAM_SPI_SD_TX,(DWORD)buff);
		dma_set_transfer_mode(DMA1, DMA_STREAM_SPI_SD_TX, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
		dma_enable_memory_increment_mode(DMA1,DMA_STREAM_SPI_SD_TX);
#endif

	}

	/* Enable DMA Channels */
	dma_enable_stream(DMA1,DMA_STREAM_SPI_SD_RX);
	dma_enable_stream(DMA1,DMA_STREAM_SPI_SD_TX);

	/* Enable SPI TX/RX request */
	spi_enable_rx_dma(SPI_SD);
	spi_enable_tx_dma(SPI_SD);

	/* Wait until DMA1_Channel 3 Transfer Complete */
	while (! dma_get_interrupt_flag(DMA1,DMA_STREAM_SPI_SD_TX,DMA_TCIF));

	/* Wait until DMA1_Channel 2 Receive Complete */
	while (! dma_get_interrupt_flag(DMA1,DMA_STREAM_SPI_SD_RX,DMA_TCIF));

	/* Disable DMA Channels */
	dma_disable_stream(DMA1,DMA_STREAM_SPI_SD_RX);
	dma_disable_stream(DMA1,DMA_STREAM_SPI_SD_TX);

	/* Disable SPI RX/TX requests */
	spi_disable_rx_dma(SPI_SD);
	spi_disable_tx_dma(SPI_SD);
}
#endif /* STM32_SD_USE_DMA */


/*---------------------------------------------------------------------------*/
/** @brief Power Control and Interface-Initialization

All peripherals are initialised and the power is turned on to the card.
*/
static void power_on (void)
{
	volatile BYTE dummyread;

	/* Enable GPIO clock for CS */
	rcc_peripheral_enable_clock(&RCC_GPIO, RCC_GPIO_PORT_CS);
	/* Enable SPI clock, SPI1: APB2, SPI2: APB1 */
	rcc_peripheral_enable_clock(&RCC_SPI, RCC_SPI_SD);

    
	card_power(1);
    
	socket_cp_init();//Empty return
	socket_wp_init();//Empty return.

	for (Timer1 = 25; Timer1; );	/* Wait for 250ms */
	//for (uint32_t Timer = 25000; Timer>0;Timer--);	/* Wait for 250ms */

	/* Configure I/O for Flash Chip select */
	gpio_mode_setup(GPIO_PORT_CS, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIOCS);
	gpio_set_output_options(GPIO_PORT_CS, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIOCS);

	/* De-select the Card: Chip Select high */
	DESELECT();

	/* Configure SPI pins: SCK MISO and MOSI with alternate function push-down */
	gpio_mode_setup(GPIO_PORT_SPI_SD, GPIO_MODE_AF, GPIO_PUPD_PULLUP,
			    GPIOSPI_SD_SCK | GPIOSPI_SD_MOSI | GPIOSPI_SD_MISO);
	gpio_set_af(GPIO_PORT_SPI_SD, GPIO_AF_SD,
			    GPIOSPI_SD_SCK | GPIOSPI_SD_MOSI | GPIOSPI_SD_MISO);
    
	/* Clear all SPI and associated RCC registers */
	spi_reset(SPI_SD);

	/* SPI configuration */
	spi_init_master(SPI_SD, SPI_BaudRatePrescaler_SPI_SD, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
			SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
	spi_set_full_duplex_mode(SPI_SD);
	spi_set_master_mode(SPI_SD);
	spi_disable_crc(SPI_SD);
	spi_enable_software_slave_management(SPI_SD);
	spi_set_nss_high(SPI_SD);

	spi_enable(SPI_SD);

	/* drain SPI */
	while (!(SPI_SR(SPI_SD) & SPI_SR_TXE));
	dummyread = SPI_DR(SPI_SD);

#ifdef STM32_SD_USE_DMA
	/* enable DMA clock */
	rcc_peripheral_enable_clock(&RCC_AHB1ENR, RCC_AHB1ENR_DMA1EN);
#endif
	(void)dummyread;
}

/*---------------------------------------------------------------------------*/
/** @brief Power Off and Peripheral Disable

All peripherals are disabled and the power is turned off to the card.
*/
static void power_off (void)
{
  
	if (!(Stat & STA_NOINIT)) {
		SELECT();
		wait_ready();
		release_spi();
	}

	spi_disable(SPI_SD);
	rcc_peripheral_disable_clock(&RCC_SPI, RCC_SPI_SD);
    
	//All SPI-Pins to input with weak internal pull-downs
	gpio_mode_setup(GPIO_PORT_SPI_SD, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN,
			    GPIOSPI_SD_SCK | GPIOSPI_SD_MISO | GPIOSPI_SD_MOSI);

	card_power(0);

	Stat |= STA_NOINIT;		/* Set STA_NOINIT */
}

/*---------------------------------------------------------------------------*/
/** @brief Receive a Data Block from the Card

Can be done with DMA or programmed.

@param *buff: BYTE 512 byte data block to store received data 
@param btr: UINT Byte count (must be multiple of 4)
*/
static BOOL rcvr_datablock (
        BYTE *buff,                     /* Data buffer to store received data */
        UINT btr                        /* Byte count (must be multiple of 4) */
)
{
        BYTE token;


        Timer1 = 10;
        do {                                                    /* Wait for data packet in timeout of 100ms */
                token = rcvr_spi();
        } while ((token == 0xFF) && Timer1);
        if(token != 0xFE) return FALSE; /* If not valid data token, return with error */

#ifdef STM32_SD_USE_DMA
        stm32_dma_transfer( TRUE, buff, btr );
#else
        do {                                                    /* Receive the data block into buffer */
                rcvr_spi_m(buff++);
                rcvr_spi_m(buff++);
                rcvr_spi_m(buff++);
                rcvr_spi_m(buff++);
        } while (btr -= 4);
#endif /* STM32_SD_USE_DMA */

        rcvr_spi();                                             /* Discard CRC */
        rcvr_spi();

        return TRUE;                                    /* Return with success */
}

/*---------------------------------------------------------------------------*/
/** @brief Send a data packet to MMC

Only compiled if the filesystem is writeable.

@param *buff: BYTE 512 byte data block to be transmitted
@param token: BYTE Data/Stop token
*/
#if _FS_READONLY == 0
static BOOL xmit_datablock (
        const BYTE *buff,       /* 512 byte data block to be transmitted */
        BYTE token                      /* Data/Stop token */
)
{
        BYTE resp;
#ifndef STM32_SD_USE_DMA
        BYTE wc;
#endif

        if (wait_ready() != 0xFF) return FALSE;

        xmit_spi(token);                                        /* transmit data token */
        if (token != 0xFD) {    /* Is data token */

#ifdef STM32_SD_USE_DMA
                stm32_dma_transfer( FALSE, buff, 512 );
#else
                wc = 0;
                do {                                                    /* transmit the 512 byte data block to MMC */
                        xmit_spi(*buff++);
                        xmit_spi(*buff++);
                } while (--wc);
#endif /* STM32_SD_USE_DMA */

                xmit_spi(0xFF);                                 /* CRC (Dummy) */
                xmit_spi(0xFF);
                resp = rcvr_spi();                              /* Receive data response */
                if ((resp & 0x1F) != 0x05)              /* If not accepted, return with error */
                        return FALSE;
        }

        return TRUE;
}
#endif /* _READONLY */

/*---------------------------------------------------------------------------*/
/** @brief Send a command packet to MMC

@param cmd: BYTE Command byte
@param arg: DWORD argument for command
@returns BYTE response
*/
static BYTE send_cmd (
        BYTE cmd,               /* Command byte */
        DWORD arg               /* Argument */
)
{
        BYTE n, res;


        if (cmd & 0x80) {       /* ACMD<n> is the command sequence of CMD55-CMD<n> */
                cmd &= 0x7F;
                res = send_cmd(CMD55, 0);
                if (res > 1) return res;
        }

        /* Select the card and wait for ready */
        SELECT();
        if (wait_ready() != 0xFF) {
                return 0xFF;
        }

        /* Send command packet */
        xmit_spi(cmd);                                          /* Start + Command index */
        xmit_spi((BYTE)(arg >> 24));            /* Argument[31..24] */
        xmit_spi((BYTE)(arg >> 16));            /* Argument[23..16] */
        xmit_spi((BYTE)(arg >> 8));                     /* Argument[15..8] */
        xmit_spi((BYTE)arg);                            /* Argument[7..0] */
        n = 0x01;                                                       /* Dummy CRC + Stop */
        if (cmd == CMD0) n = 0x95;                      /* Valid CRC for CMD0(0) */
        if (cmd == CMD8) n = 0x87;                      /* Valid CRC for CMD8(0x1AA) */
        xmit_spi(n);

        /* Receive command response */
        if (cmd == CMD12) rcvr_spi();           /* Skip a stuff byte when stop reading */

        n = 10;                                                         /* Wait for a valid response in timeout of 10 attempts */
        do
                res = rcvr_spi();
        while ((res & 0x80) && --n);

        return res;                     /* Return with the response value */
}

/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/** @brief Initialize Disk Drive

@param[in] drv: BYTE Physical drive number (only 0 allowed)
*/
DSTATUS disk_initialize (
        BYTE drv                /* Physical drive number (0) */
)
{
        BYTE n, cmd, ty, ocr[4];

        if (drv) return STA_NOINIT;                     /* Supports only single drive */
        if (Stat & STA_NODISK) return Stat;     /* No card in the socket */

        power_on();                                                     /* Force socket power on and initialize interface */
        interface_speed(INTERFACE_SLOW);
        for (n = 10; n; n--) rcvr_spi();        /* 80 dummy clocks */

        ty = 0;
        if (send_cmd(CMD0, 0) == 1) {                   /* Enter Idle state */
                Timer1 = 100;                                           /* Initialization timeout of 1000 milliseconds */
                if (send_cmd(CMD8, 0x1AA) == 1) {       /* SDHC */
                        for (n = 0; n < 4; n++) ocr[n] = rcvr_spi();            /* Get trailing return value of R7 response */
                        if (ocr[2] == 0x01 && ocr[3] == 0xAA) {                         /* The card can work at VDD range of 2.7-3.6V */
                                while (Timer1 && send_cmd(ACMD41, 1UL << 30));  /* Wait for leaving idle state (ACMD41 with HCS bit) */
                                if (Timer1 && send_cmd(CMD58, 0) == 0) {                /* Check CCS bit in the OCR */
                                        for (n = 0; n < 4; n++) ocr[n] = rcvr_spi();
                                        ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;
                                }
                        }
                } else {                                                        /* SDSC or MMC */
                        if (send_cmd(ACMD41, 0) <= 1)   {
                                ty = CT_SD1; cmd = ACMD41;      /* SDSC */
                        } else {
                                ty = CT_MMC; cmd = CMD1;        /* MMC */
                        }
                        while (Timer1 && send_cmd(cmd, 0));                     /* Wait for leaving idle state */
                        if (!Timer1 || send_cmd(CMD16, 512) != 0)       /* Set R/W block length to 512 */
                                ty = 0;
                }
        }
        CardType = ty;
        release_spi();

        if (ty) {                       /* Initialization succeeded */
                Stat &= ~STA_NOINIT;            /* Clear STA_NOINIT */
                interface_speed(INTERFACE_FAST);
        } else {                        /* Initialization failed */
                power_off();
        }

        return Stat;
}

/*---------------------------------------------------------------------------*/
/** @brief Get Disk Status

@param[in] drv: BYTE Physical drive number (only 0 allowed)
@returns DSTATUS
*/
DSTATUS disk_status (
        BYTE drv                /* Physical drive number (0) */
)
{
        if (drv) return STA_NOINIT;             /* Supports only single drive */
        return Stat;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

// TODO quick & dirty
int8_t SD_ReadSectors(uint8_t *buff, uint32_t sector, uint32_t count)
{
  if (!(CardType & CT_BLOCK)) sector *= 512;      /* Convert to byte address if needed */

  if (send_cmd(CMD18, sector) == 0) {     /* READ_MULTIPLE_BLOCK */
    do {
      if (!rcvr_datablock(buff, 512)) {
        break;
      }
      buff += 512;
    } while (--count);
    send_cmd(CMD12, 0);                             /* STOP_TRANSMISSION */
  }

  release_spi();

  return 0;
}

/*---------------------------------------------------------------------------*/
/** @brief Read Disk Sectors

@param[in] drv: BYTE Physical drive number (only 0 allowed)
@param[in] *buff: BYTE Pointer to buffer
@param[in] sector: DWORD starting sector number
@param[in] count: BYTE number of sectors to read
@returns DRESULT success (RES_OK) or fail.
*/
DRESULT disk_read (
        BYTE drv,                       /* Physical drive number (0) */
        BYTE *buff,                     /* Pointer to the data buffer to store read data */
        DWORD sector,           /* Start sector number (LBA) */
        BYTE count                      /* Sector count (1..255) */
)
{
        if (drv || !count) return RES_PARERR;
        if (Stat & STA_NOINIT) return RES_NOTRDY;

        if (!(CardType & CT_BLOCK)) sector *= 512;      /* Convert to byte address if needed */

        if (count == 1) {       /* Single block read */
                if (send_cmd(CMD17, sector) == 0)       { /* READ_SINGLE_BLOCK */
                        if (rcvr_datablock(buff, 512)) {
                                count = 0;
                        }
                }
        }
        else {                          /* Multiple block read */
                if (send_cmd(CMD18, sector) == 0) {     /* READ_MULTIPLE_BLOCK */
                        do {
                                if (!rcvr_datablock(buff, 512)) {
                                        break;
                                }
                                buff += 512;
                        } while (--count);
                        send_cmd(CMD12, 0);                             /* STOP_TRANSMISSION */
                }
        }
        release_spi();

        return count ? RES_ERROR : RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _FS_READONLY == 0

// TODO quick & dirty
int8_t SD_WriteSectors(uint8_t *buff, uint32_t sector, uint32_t count)
{
  if (!(CardType & CT_BLOCK)) sector *= 512;      /* Convert to byte address if needed */
  if (CardType & CT_SDC) send_cmd(ACMD23, count);
  if (send_cmd(CMD25, sector) == 0) {     /* WRITE_MULTIPLE_BLOCK */
    do {
      if (!xmit_datablock(buff, 0xFC)) break;
      buff += 512;
    } while (--count);
    if (!xmit_datablock(0, 0xFD))   /* STOP_TRAN token */
      count = 1;
  }

  release_spi();

  return 0;
}

/*---------------------------------------------------------------------------*/
/** @brief Write Disk Sectors

@param[in] drv: BYTE Physical drive number (only 0 allowed)
@param[in] *buff: BYTE Pointer to buffer
@param[in] sector: DWORD starting sector number
@param[in] count: BYTE number of sectors to read
@returns DRESULT success (RES_OK) or fail.
*/
DRESULT disk_write (
        BYTE drv,                       /* Physical drive number (0) */
        const BYTE *buff,       /* Pointer to the data to be written */
        DWORD sector,           /* Start sector number (LBA) */
        BYTE count                      /* Sector count (1..255) */
)
{
        if (drv || !count) return RES_PARERR;
        if (Stat & STA_NOINIT) return RES_NOTRDY;
        if (Stat & STA_PROTECT) return RES_WRPRT;

        if (!(CardType & CT_BLOCK)) sector *= 512;      /* Convert to byte address if needed */

        if (count == 1) {       /* Single block write */
                if ((send_cmd(CMD24, sector) == 0)      /* WRITE_BLOCK */
                        && xmit_datablock(buff, 0xFE))
                        count = 0;
        }
        else {                          /* Multiple block write */
                if (CardType & CT_SDC) send_cmd(ACMD23, count);
                if (send_cmd(CMD25, sector) == 0) {     /* WRITE_MULTIPLE_BLOCK */
                        do {
                                if (!xmit_datablock(buff, 0xFC)) break;
                                buff += 512;
                        } while (--count);
                        if (!xmit_datablock(0, 0xFD))   /* STOP_TRAN token */
                                count = 1;
                }
        }
        release_spi();

        return count ? RES_ERROR : RES_OK;
}
#endif /* _READONLY == 0 */



/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/** @brief Disk I/O Control

@param[in] drv: BYTE Physical drive number (only 0 allowed)
@param[in] ctrl: BYTE Control Code.
                    CTRL_POWER (0=on,1=off,2=get setting)
                    CTRL_SYNC wait for writes to complete
                    GET_SECTOR_COUNT iumber of sectors on disk
                    GET_SECTOR_SIZE
                    GET_BLOCK_SIZE
                    MMC_GET_TYPE
                    MMC_GET_CSD     ???
                    MMC_GET_CID     Card ID
                    MMC_GET_OCR     Operating Conditions Register
                    MMC_GET_SDSTAT  Card status
@param[in] *buff: BYTE Pointer to buffer
@returns DRESULT success (RES_OK) or fail (RES_ERROR).
*/
#if (STM32_SD_DISK_IOCTRL == 1)
DRESULT disk_ioctl (
        BYTE drv,               /* Physical drive number (0) */
        BYTE ctrl,              /* Control code */
        void *buff              /* Buffer to send/receive control data */
)
{
        DRESULT res;
        BYTE n, csd[16], *ptr = (BYTE *)buff;
        WORD csize;

        if (drv) return RES_PARERR;

        res = RES_ERROR;

        if (ctrl == CTRL_POWER) {
                switch (*ptr) {
                case 0:         /* Sub control code == 0 (POWER_OFF) */
                        if (chk_power())
                                power_off();            /* Power off */
                        res = RES_OK;
                        break;
                case 1:         /* Sub control code == 1 (POWER_ON) */
                        power_on();                             /* Power on */
                        res = RES_OK;
                        break;
                case 2:         /* Sub control code == 2 (POWER_GET) */
                        *(ptr+1) = (BYTE)chk_power();
                        res = RES_OK;
                        break;
                default :
                        res = RES_PARERR;
                }
        }
        else {
                if (Stat & STA_NOINIT) return RES_NOTRDY;

                switch (ctrl) {
                case CTRL_SYNC :                /* Make sure that no pending write process */
                        SELECT();
                        if (wait_ready() == 0xFF)
                                res = RES_OK;
                        break;

                case GET_SECTOR_COUNT : /* Get number of sectors on the disk (DWORD) */
                        if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
                                if ((csd[0] >> 6) == 1) {       /* SDC version 2.00 */
                                        csize = csd[9] + ((WORD)csd[8] << 8) + 1;
                                        *(DWORD*)buff = (DWORD)csize << 10;
                                } else {                                        /* SDC version 1.XX or MMC*/
                                        n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
                                        csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
                                        *(DWORD*)buff = (DWORD)csize << (n - 9);
                                }
                                res = RES_OK;
                        }
                        break;

                case GET_SECTOR_SIZE :  /* Get R/W sector size (WORD) */
                        *(WORD*)buff = 512;
                        res = RES_OK;
                        break;

                case GET_BLOCK_SIZE :   /* Get erase block size in unit of sector (DWORD) */
                        if (CardType & CT_SD2) {        /* SDC version 2.00 */
                                if (send_cmd(ACMD13, 0) == 0) { /* Read SD status */
                                        rcvr_spi();
                                        if (rcvr_datablock(csd, 16)) {                          /* Read partial block */
                                                for (n = 64 - 16; n; n--) rcvr_spi();   /* Purge trailing data */
                                                *(DWORD*)buff = 16UL << (csd[10] >> 4);
                                                res = RES_OK;
                                        }
                                }
                        } else {                                        /* SDC version 1.XX or MMC */
                                if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {      /* Read CSD */
                                        if (CardType & CT_SD1) {        /* SDC version 1.XX */
                                                *(DWORD*)buff = (((csd[10] & 63) << 1) + ((WORD)(csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
                                        } else {                                        /* MMC */
                                                *(DWORD*)buff = ((WORD)((csd[10] & 124) >> 2) + 1) * (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
                                        }
                                        res = RES_OK;
                                }
                        }
                        break;

                case MMC_GET_TYPE :             /* Get card type flags (1 byte) */
                        *ptr = CardType;
                        res = RES_OK;
                        break;

                case MMC_GET_CSD :              /* Receive CSD as a data block (16 bytes) */
                        if (send_cmd(CMD9, 0) == 0              /* READ_CSD */
                                && rcvr_datablock(ptr, 16))
                                res = RES_OK;
                        break;

                case MMC_GET_CID :              /* Receive CID as a data block (16 bytes) */
                        if (send_cmd(CMD10, 0) == 0             /* READ_CID */
                                && rcvr_datablock(ptr, 16))
                                res = RES_OK;
                        break;

                case MMC_GET_OCR :              /* Receive OCR as an R3 resp (4 bytes) */
                        if (send_cmd(CMD58, 0) == 0) {  /* READ_OCR */
                                for (n = 4; n; n--) *ptr++ = rcvr_spi();
                                res = RES_OK;
                        }
                        break;

                case MMC_GET_SDSTAT :   /* Receive SD status as a data block (64 bytes) */
                        if (send_cmd(ACMD13, 0) == 0) { /* SD_STATUS */
                                rcvr_spi();
                                if (rcvr_datablock(ptr, 64))
                                        res = RES_OK;
                        }
                        break;

                default:
                        res = RES_PARERR;
                }

                release_spi();
        }

        return res;
}
#endif /* _USE_IOCTL != 0 */


/*---------------------------------------------------------------------------*/
/** @brief Device Timer Interrupt Procedure

This function must be called in period of 10ms, generally by the systick ISR.
It counts down two timers and checks for write protect and card presence

*/
void sdPoll10ms()
{
        static DWORD pv;
        DWORD ns;
        BYTE n, s;


        n = Timer1;                /* 100Hz decrement timers */
        if (n) Timer1 = --n;
        n = Timer2;
        if (n) Timer2 = --n;

        ns = pv;
        pv = socket_is_empty() | socket_is_write_protected();   /* Sample socket switch */

        if (ns == pv) {                         /* Have contacts stabled? */
                s = Stat;

                if (pv & socket_state_mask_wp)      /* WP is H (write protected) */
                        s |= STA_PROTECT;
                else                                /* WP is L (write enabled) */
                        s &= ~STA_PROTECT;

                if (pv & socket_state_mask_cp)      /* INS = H (Socket empty) */
                        s |= (STA_NODISK | STA_NOINIT);
                else                                /* INS = L (Card inserted) */
                        s &= ~STA_NODISK;

                Stat = s;
        }
}

// TODO everything here should not be in the driver layer ...

FATFS g_FATFS_Obj;
#if defined(DEBUG)
FIL g_telemetryFile = {0};
#endif

uint32_t sdMounted()
{
  return g_FATFS_Obj.fs_type != 0;
}

uint32_t sdInit()
{
  return (f_mount(0, &g_FATFS_Obj) == FR_OK);
}

void sdDone()
{
  if (sdMounted()) {
    f_mount(0, 0); // unmount SD
  }
}

char *_fat_fgets(char *s, int size, FILE *stream) {
    return f_gets(s, size, (FIL *)stream);
}
