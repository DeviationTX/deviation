#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/msc.h>

#include "common.h"
#include "devo_usb.h"

static const char * const usb_strings[] = {
    USB_Product_Name,
    "Deviation Disk",
    DeviationVersion
};

extern usbd_mass_storage *usb_msc_init2(usbd_device *usbd_dev,
                 uint8_t ep_in,
                 uint8_t ep_in_size,
                 uint8_t ep_out,
                 uint8_t ep_out_size,
                 const char *vendor_id,
                 const char *product_id,
                 const char *product_revision_level,
                 uint16_t block_size,
                 uint32_t block_count,
                 int (*read_block)(uint32_t lba, uint8_t *copy_to, u16 offset, u16 length),
                 int (*write_block)(uint32_t lba, const uint8_t *copy_from, u16 offset, u16 length));

static const struct usb_endpoint_descriptor msc_endp[] = {{
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x81,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = 64,
    .bInterval = 0,
}, {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x02,
    .bmAttributes = USB_ENDPOINT_ATTR_BULK,
    .wMaxPacketSize = 64,
    .bInterval = 0,
}};

static const struct usb_interface_descriptor msc_iface = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 2,
    .bInterfaceClass = USB_CLASS_MSC,
    .bInterfaceSubClass = USB_MSC_SUBCLASS_SCSI,
    .bInterfaceProtocol = USB_MSC_PROTOCOL_BBB,
    .iInterface = 0,
    .endpoint = msc_endp,
    .extra = NULL,
    .extralen = 0
};

static const struct usb_interface msc_ifaces = {
    .num_altsetting = 1,
    .altsetting = &msc_iface,
};

static const struct usb_config_descriptor msc_config_descr = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType = USB_DT_CONFIGURATION,
    .wTotalLength = 0,
    .bNumInterfaces = 1,
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = 0xC0,
    .bMaxPower = 0x32,
    .interface = &msc_ifaces,
};


#define MSC_BLOCK_SIZE 4096

#if defined USE_DEVOFS && USE_DEVOFS
    #define EMULATE_FAT 1
    #define FAT_OFFSET 3    // boot_sector + fat_sector + root_sector
    static const u8 boot[] = {
        0xeb, 0x3c, 0x90,
        'M', 'S', 'D', 'O', 'S', '5', '.', '0',
        0x00, 0x10,               // Bytes per Sector
        0x10,                     // Sectors per Cluster
        0x01, 0x00,               // Reserved Sectors
        0x01,                     // Number of FATs
        0x80, 0x00,               // Root Entries
        0x13, 0x00,               // Small Sectors
        0xF8,                     // Media Type
        0x01, 0x00,               // Sectors per FAT
        0x20, 0x00,               // Sectors per Track
        0x40, 0x00,               // Number of Heads
        0x00, 0x00, 0x00, 0x00,   // Hidden Sectors
        0x00, 0x00, 0x00, 0x00,   // Large Sectors
        0x80,                     // Physical Disk Number
        0x00,                     // Current Head
        0x29,                     // Signature
        0xC2, 0x72, 0x31, 0x29,   // Volume Serial Number
        'D', 'E', 'V', 'O', 'F', 'S', 0, 0, 0, 0, 0,
        'F', 'A', 'T', '1', '2', ' ', ' ', ' ',
    };

    static const u8 fat[] = {
        0xF8, 0x0F | 0xF0, 0xFF,
        0xFF, 0x0F
    };
//   0x03, 0x00 | 0x40, 0x00,
//   0x05, 0x00 | 0x60, 0x00,
//   0x07, 0x00 | 0x80, 0x00,
//   0x09, 0x00 | 0xA0, 0x00,
//   0x0B, 0x00 | 0xC0, 0x00,
//   0x0D, 0x00 | 0xE0, 0x00,
//   0x0F, 0x00 | 0x00, 0x01,
//   0x11, 0x00 | 0xF0, 0xFF,

    static const u8 root[] = {
        'D',  'E',  'V',  'O',  ' ',  ' ',  ' ',  ' ',  'F',  'S',  ' ',  0x20, 0x18, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00,
    };
#else
    #define EMULATE_FAT 0
    #define FAT_OFFSET 0
#endif

#if defined HAS_FLASH_DETECT && HAS_FLASH_DETECT
    uint32_t Mass_Block_Count = 0;
#else
    uint32_t Mass_Block_Count = FAT_OFFSET + SPIFLASH_SECTORS - SPIFLASH_SECTOR_OFFSET;
#endif


/*******************************************************************************
* Function Name  : MAL_Write
* Description    : Write sectors
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
int MSC_Write(uint32_t lba, const u8 *Writebuff, uint16_t offset, uint16_t Transfer_Length)
{
    uint32_t Memory_Offset = lba * MSC_BLOCK_SIZE + offset;
#if EMULATE_FAT
    if (Memory_Offset + Transfer_Length < (0x1000 * FAT_OFFSET)) {
        return 0;
    }
    if (Memory_Offset < (0x1000 * FAT_OFFSET)) {
        Transfer_Length -= ((0x1000 * FAT_OFFSET) - Memory_Offset);
        Writebuff += ((0x1000 * FAT_OFFSET) - Memory_Offset);
        Memory_Offset = 0x1000 * FAT_OFFSET;
    }
#endif

    if (offset == 0) {
        STORAGE_EraseSector(Memory_Offset + ((SPIFLASH_SECTOR_OFFSET - FAT_OFFSET) * 0x1000));
    }
    STORAGE_WriteBytes(Memory_Offset + ((SPIFLASH_SECTOR_OFFSET - FAT_OFFSET) * 0x1000), Transfer_Length, (u8 *)Writebuff);

    return 0;
}

/*******************************************************************************
* Function Name  : MAL_Read
* Description    : Read sectors
* Input          : None
* Output         : None
* Return         : Buffer pointer
*******************************************************************************/
int MSC_Read(uint32_t lba, u8 *Readbuff, uint16_t offset, uint16_t Transfer_Length)
{
    uint32_t Memory_Offset;

    Memory_Offset = lba * MSC_BLOCK_SIZE + offset;

#if EMULATE_FAT
      if (Memory_Offset < (FAT_OFFSET * 0x1000)) {
          static const struct {
              u16 addr;
              u16 size;
              const u8* mem;
          } map[3] = {
              {0, sizeof(boot), boot},
              {4096, sizeof(fat), fat},
              {8192, sizeof(root), root}
          };
          memset(Readbuff, 0, Transfer_Length);
          for (int i = 0; i < 3; i++) {
              u32 end = map[i].addr + map[i].size;
              if (Memory_Offset >= map[i].addr && Memory_Offset < end) {
                  int size = end - Memory_Offset;
                  if (Transfer_Length < size) {
                      size = Transfer_Length;
                  }
                  memcpy(Readbuff, map[i].mem + (Memory_Offset - map[i].addr), size);
              }
          }
          if (0x1FE >= Memory_Offset && 0x1FE < (Memory_Offset + Transfer_Length)) {
              // printf("Setting %06x to %02x\n", 0x1FE - Memory_Offset, 0x55);
              ((u8 *)Readbuff)[0x1FE - Memory_Offset] = 0x55;
          }
          if (0x1FF >= Memory_Offset && 0x1FF < (Memory_Offset + Transfer_Length)) {
              // printf("Setting %06x to %02x\n", 0x1FF - Memory_Offset, 0xAA);
              ((u8 *)Readbuff)[0x1FF - Memory_Offset] = 0xAA;
          }
          return 0;
      }
#endif
    STORAGE_ReadBytes(Memory_Offset + ((SPIFLASH_SECTOR_OFFSET - FAT_OFFSET) * 0x1000), Transfer_Length, (u8*)Readbuff);

    return 0;
}

static void MSC_Init()
{
    usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev_descr, &msc_config_descr,
        usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));

    usb_msc_init2(usbd_dev, 0x81, 0x64, 0x02, 0x64,
        "ST", "SD Flash Disk", "1.0",
        MSC_BLOCK_SIZE, Mass_Block_Count,
        MSC_Read, MSC_Write);
}

void MSC_Enable()
{
    USB_Enable(1);
    MSC_Init();
}

void MSC_Disable()
{
    USB_Disable();
}
