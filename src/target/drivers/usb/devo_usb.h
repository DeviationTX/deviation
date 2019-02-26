#ifndef _DEVO_USB_H_
#define _DEVO_USB_H_

extern void MSC_Init();
extern void HID_Init();

extern usbd_device *usbd_dev;

extern uint8_t usbd_control_buffer[128];

extern const struct usb_device_descriptor dev_descr;

#define USB_STRING_COUNT 3
extern const char *usb_strings[];

#endif
