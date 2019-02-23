/*
 * Copyright (C) 2016 Dave Hylands <dhylands@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _USB_UART_H_
#define _USB_UART_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void usb_vcp_init(void);

bool usb_vcp_is_connected(void);

uint16_t usb_vcp_avail(void);
int usb_vcp_recv_byte(void);
void usb_vcp_send_byte(uint8_t ch);
void usb_vcp_send_strn(const char *str, size_t len);
void usb_vcp_send_strn_cooked(const char *str, size_t len);

#endif  // _USB_UART_H_
