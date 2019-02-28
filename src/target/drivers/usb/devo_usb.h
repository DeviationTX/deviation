#ifndef _DEVO_USB_H_
#define _DEVO_USB_H_

extern usbd_device *usbd_dev;

extern uint8_t usbd_control_buffer[128];

extern const struct usb_device_descriptor dev_descr;
extern const char USB_Product_Name[];

#endif
