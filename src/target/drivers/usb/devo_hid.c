#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/hid.h>

#include "common.h"
#include "devo_usb.h"

static const char * const usb_strings[] = {
    USB_Product_Name,
    "Deviation GamePad",
    DeviationVersion
};

static volatile u8 usb_configured;
static const uint8_t hid_report_descriptor[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x15, 0x81,                    // LOGICAL_MINIMUM (0)
    0x25, 0x7f,                    // LOGICAL_MAXIMUM (255)
    0x75, 0x08,                    // REPORT_SIZE (8)
    0x09, 0x04,                    // USAGE (Joystick)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x82,                    //     INPUT (Data,Var,Abs,Vol)
    0xc0,                          //   END_COLLECTION
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x09, 0x32,                    //     USAGE (Z)
    0x09, 0x33,                    //     USAGE (Rx)
    0x95, 0x02,                    //     REPORT_COUNT (2)
    0x81, 0x82,                    //     INPUT (Data,Var,Abs,Vol)
    0xc0,                          //   END_COLLECTION
    0x09, 0x34,                    //   USAGE (Ry)
    0x09, 0x35,                    //   USAGE (Rz)
    0x09, 0x36,                    //   USAGE (Slider)
    0x09, 0x37,                    //   USAGE (Dial)
    0x95, 0x04,                    //   REPORT_COUNT (4)
    0x81, 0x82,                    //   INPUT (Data,Var,Abs,Vol)
    0x05, 0x09,                    //   USAGE_PAGE (Button)
    0x19, 0x01,                    //   USAGE_MINIMUM (Button 1)
    0x29, 0x04,                    //   USAGE_MAXIMUM (Button 4)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x95, 0x04,                    //   REPORT_COUNT (4)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x04,                    //   REPORT_SIZE (4)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
    0xc0                           // END_COLLECTION
};

static const struct {
    struct usb_hid_descriptor hid_descriptor;
    struct {
        uint8_t bReportDescriptorType;
        uint16_t wDescriptorLength;
    } __attribute__((packed)) hid_report;
} __attribute__((packed)) hid_function = {
    .hid_descriptor = {
        .bLength = sizeof(hid_function),
        .bDescriptorType = USB_DT_HID,
        .bcdHID = 0x0100,
        .bCountryCode = 0,
        .bNumDescriptors = 1,
    },
    .hid_report = {
        .bReportDescriptorType = USB_DT_REPORT,
        .wDescriptorLength = sizeof(hid_report_descriptor),
    }
};

static const struct usb_endpoint_descriptor hid_endpoint = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = 0x81,
    .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
    .wMaxPacketSize = 9,
    .bInterval = 0x20,
};

static const struct usb_interface_descriptor hid_iface = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 1,
    .bInterfaceClass = USB_CLASS_HID,
    .bInterfaceSubClass = 0,  // 1=BOOT, 0=no boot
    .bInterfaceProtocol = 0,  // 0=none, 1=keyboard, 2=mouse
    .iInterface = 0,
    .endpoint = &hid_endpoint,
    .extra = &hid_function,
    .extralen = sizeof(hid_function),
};

static const struct usb_interface hid_ifaces = {
    .num_altsetting = 1,
    .altsetting = &hid_iface,
};

static const struct usb_config_descriptor hid_config_descr = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType = USB_DT_CONFIGURATION,
    .wTotalLength = 0,
    .bNumInterfaces = 1,
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = 0xE0,
    .bMaxPower = 0x32,
    .interface = &hid_ifaces,
};

static enum usbd_request_return_codes hid_control_request(usbd_device *dev, struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
            void (**complete)(usbd_device *, struct usb_setup_data *))
{
    (void)complete;
    (void)dev;

    if ((req->bmRequestType != 0x81) ||
        (req->bRequest != USB_REQ_GET_DESCRIPTOR) ||
        (req->wValue != 0x2200))
        return USBD_REQ_NOTSUPP;

    /* Handle the HID report descriptor. */
    *buf = (uint8_t *)hid_report_descriptor;
    *len = sizeof(hid_report_descriptor);

    return USBD_REQ_HANDLED;
}

static void hid_set_config(usbd_device *dev, uint16_t wValue)
{
    (void)wValue;

    // Max 9 bytes to send. 8 analog channels + 1 bytes for 4 switches
    usbd_ep_setup(dev, 0x81, USB_ENDPOINT_ATTR_INTERRUPT, 9, NULL);

    usbd_register_control_callback(
                dev,
                USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
                USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
                hid_control_request);

    usb_configured = 1;
}

static void HID_Init()
{
    usbd_dev = usbd_init(&st_usbfs_v1_usb_driver, &dev_descr, &hid_config_descr,
        usb_strings, 3, usbd_control_buffer, sizeof(usbd_control_buffer));

    usbd_register_set_config_callback(usbd_dev, hid_set_config);
}

void HID_Write(s8 *packet, u8 size)
{
    if (usb_configured) {
        usbd_ep_write_packet(usbd_dev, 0x81, packet, size);
    }
}

void HID_Enable() {
    usb_configured = 0;
    USB_Enable(1);
    HID_Init();
}

void HID_Disable() {
    USB_Disable();
}
