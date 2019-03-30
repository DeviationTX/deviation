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

#if SUPPORT_CRSF_CONFIG

#include "crsf.h"

#define CRSF_MAX_PARAMS  45   // one extra required, max observed is 40 in Nano RX
#ifndef EMULATOR
crsf_param_t crsf_params[CRSF_MAX_PARAMS];
#else
#include "protocol/crsf_params_tx._c"
#include "protocol/crsf_params_rx._c"
crsf_param_t *crsf_params = tx_crsf_params;
#endif

static struct crsfdevice_page * const mp = &pagemem.u.crsfdevice_page;
static struct crsfdevice_obj * const gui = &gui_objs.u.crsfdevice;

static u32 last_update;
static u32 read_timeout;
static u16 current_selected = 0;
static u8 current_folder = 0;
static u8 current_folder_count;
static u8 params_loaded;     // if not zero, number displayed so far for current device
static u8 device_idx;   // current device index
static u8 next_param;   // parameter and chunk currently being read
static u8 next_chunk;

static struct {
    crsf_param_t *param;
    u32 time;
    u16 timeout;
    u8  dialog;
    u8  redraw;
} command;

#define CRSF_MAX_CHUNK_SIZE   58   // 64 - header - type - destination - origin
#define CRSF_MAX_CHUNKS        4   // not in specification. Max observed is 3 for Nano RX
static char recv_param_buffer[CRSF_MAX_CHUNKS * CRSF_MAX_CHUNK_SIZE];
static char *recv_param_ptr;

#define SEND_MSG_BUF_SIZE  64      // don't send more than one chunk
static u8 send_msg_buffer[SEND_MSG_BUF_SIZE];
static volatile int send_msg_buf_count;     // tx data available semaphore with CRSF protocol

static char *next_string;

#define MIN(a, b) ((a) < (b) ? a : b)

static void crsfdevice_init() {
    next_param = 1;
    next_chunk = 0;
    recv_param_ptr = recv_param_buffer;
#ifndef EMULATOR
    params_loaded = 0;
    next_string = mp->strings;
    memset(crsf_params, 0, sizeof crsf_params);
#endif
}

crsf_param_t *current_param(int absrow) {
    int idx = 0;
    crsf_param_t *param = crsf_params;

    while (param->id && (param->parent != current_folder || param->hidden)) param++;
    while (idx++ != absrow) {
        param++;
        while (param->id && (param->parent != current_folder || param->hidden)) param++;
    }
    return param;
}

static const char *current_text(crsf_param_t *param) {
    const char *p = (const char *)param->value;
    int i = param->u.text_sel;
    while (i--) while (*p++) {}
    return p;
}

static const char *crsf_name_cb(guiObject_t *obj, const void *data)
{
    (void)obj;

    crsf_param_t *param = (crsf_param_t *)data;

    if (param->id == 0) return "--";

    return (const char *)param->name;
}

static const char *crsf_value_cb(guiObject_t *obj, const void *data)
{
    (void)obj;

    crsf_param_t *param = (crsf_param_t *)data;

    switch (param->type) {
    case UINT8:
    case UINT16:
    case FLOAT:
        snprintf(tempstring, sizeof tempstring, "%d", (unsigned)param->value);
        break;
    case INT8:
    case INT16:
        snprintf(tempstring, sizeof tempstring, "%d", (int)param->value);
        break;
    case TEXT_SELECTION:
        return current_text(param);
        break;
    case STRING:
        if (param->value && *(char *)param->value)
            return (const char *)param->value;
        else
            return (const char *)param->default_value;
        break;
    case INFO:
        return (const char *)param->value;
        break;
    case COMMAND:
        return (const char *)param->s.info;
        break;
    case OUT_OF_RANGE:
    default:
        break;
    }
    return (const char *)tempstring;
}

static u8 count_params_loaded() {
    int i;
    for (i=0; i < CRSF_MAX_PARAMS; i++)
        if (crsf_params[i].id == 0) break;
    return i;
}

static int folder_rows(int folder) {
    int count = 0;
    crsf_param_t *param = crsf_params;

    while (param->id) {
        if (param->parent == folder && !param->hidden) count += 1;
        param += 1;
    }
    return count;
}

void button_press(guiObject_t *obj, const void *data)
{
    (void)obj;
    crsf_param_t *param = (crsf_param_t *)data;

    if (param->u.text_sel == param->min_value)
        param->u.text_sel = param->max_value;
    else
        param->u.text_sel = param->min_value;

    param->changed = 1;
}

static void command_press(guiObject_t *obj, s8 press_type, const void *data)
{
    (void)obj;

    if (press_type != -1) {
        return;
    }

    crsf_param_t *param = (crsf_param_t *)data;

    if (param->u.status == READY) {
        CRSF_send_command(param, START);
    }
}

static const char *value_textsel(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    crsf_param_t *param = (crsf_param_t *)data;
    u8 changed = 0;

    param->u.text_sel = GUI_TextSelectHelper(param->u.text_sel,
                            param->min_value, param->max_value,
                            dir, 1, 1, &changed);

    if (changed) param->changed = 1;
    return current_text(param);
}

static const char *value_numsel(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    crsf_param_t *param = (crsf_param_t *)data;
    u8 changed = 0;

    param->value = (void *)GUI_TextSelectHelper((int)param->value,
                                param->min_value, param->max_value,
                                dir, param->step, 10*param->step, &changed);

    if (changed) param->changed = 1;

    snprintf(tempstring, sizeof tempstring, "%d", (int)param->value);
    if (param->type == FLOAT && param->u.point > 0) {
        int pos = strlen(tempstring) - param->u.point;
        memmove(&tempstring[pos+1], &tempstring[pos], param->u.point+1);
        tempstring[pos] = '.';
    }
    return tempstring;
}

void show_page(int folder);

static void folder_cb(struct guiObject *obj, s8 press_type, const void *data)
{
    (void)obj;
    if (press_type != -1) {
        return;
    }

    crsf_param_t *param = (crsf_param_t *)data;

    current_folder = param->id;
    show_page(current_folder);
}

static void stredit_done_cb(guiObject_t *obj, void *data)  // devo8 doesn't handle cancel/discard properly,
{
    crsf_param_t *param = (crsf_param_t *)data;

    GUI_RemoveObj(obj);
    if (param) {  // Keyboard sets to null if changes discarded
        strlcpy((char *)param->value, (const char *)tempstring, param->u.string_max_len);
        param->changed = 1;
    }
    show_page(current_folder);
}

static void stredit_cb(struct guiObject *obj, s8 press_type, const void *data)
{
    (void)obj;
    if (press_type != -1) {
        return;
    }

    crsf_param_t *param = (crsf_param_t *)data;

    PAGE_SetModal(1);
    PAGE_RemoveAllObjects();
    tempstring_cpy((const char *)param->value);
    GUI_CreateKeyboard(&gui->keyboard, KEYBOARD_ALPHA, tempstring, param->u.string_max_len,
            stredit_done_cb, param);
}

void PAGE_CRSFDeviceEvent() {
    // update page as parameter info is received
    // until all parameters loaded
    if (CLOCK_getms() - last_update > 300) {
        u8 params_count = count_params_loaded();
        if (params_loaded != params_count) {
            params_loaded = params_count;
            show_page(current_folder);
        }
        last_update = CLOCK_getms();
    }

    // commands may require interaction through dialog
    if (command.dialog == 1) {
        command.dialog = 0;
        PAGE_CRSFdialog(command.param->u.status, (void *)command.param);
    } else if (command.dialog == 2) {
        command.dialog = 0;
        PAGE_CRSFdialogClose();
    }
    if (command.time && (CLOCK_getms() - command.time > command.param->timeout * 100)) {
        if (command.param->u.status != READY) {
            CRSF_send_command(command.param, POLL);
            command.time = CLOCK_getms();
        } else {
            PAGE_CRSFdialogClose();
            command.time = 0;
        }
    }
    if (command.redraw) {   // handle changes to number of menu items in folder
        command.redraw = 0;
        show_page(current_folder);
    }

    // spec calls for 2 second timeout on requests. Retry on timeout.
    if (read_timeout && (CLOCK_getms() - read_timeout > 2000)) {
        CRSF_read_param(device_idx, next_param, next_chunk);
    }
}

// Following functions queue a CRSF message for sending
// Broadcast for device info responses
void CRSF_ping_devices() {
    if (!send_msg_buf_count) {
        send_msg_buffer[0] = ADDR_MODULE;
        send_msg_buffer[1] = 4;
        send_msg_buffer[2] = TYPE_PING_DEVICES;
        send_msg_buffer[3] = ADDR_BROADCAST;
        send_msg_buffer[4] = ADDR_RADIO;
        send_msg_buffer[5] = crsf_crc8(&send_msg_buffer[2], send_msg_buffer[1]-1);
        send_msg_buf_count = 6;
    }
}

static void param_msg_header(u8 type, u8 address, u8 id) {
        send_msg_buffer[0] = ADDR_MODULE;
        send_msg_buffer[1] = 6;
        send_msg_buffer[2] = type;
        send_msg_buffer[3] = address;
        send_msg_buffer[4] = ADDR_RADIO;
        send_msg_buffer[5] = id;
}

// Request parameter info from known device
void CRSF_read_param(u8 device, u8 id, u8 chunk) {
    if (!send_msg_buf_count) {
        param_msg_header(TYPE_SETTINGS_READ, crsf_devices[device].address, id);
        send_msg_buffer[6] = chunk;
        send_msg_buffer[7] = crsf_crc8(&send_msg_buffer[2], send_msg_buffer[1]-1);
        send_msg_buf_count = 8;
        read_timeout = CLOCK_getms();
    }
}

void CRSF_set_param(crsf_param_t *param) {
    if (!send_msg_buf_count) {
        next_param = param->id;    // device responds with parameter info so prepare to receive
        next_chunk = 0;
        recv_param_ptr = recv_param_buffer;

        param_msg_header(TYPE_SETTINGS_WRITE, crsf_devices[param->device].address, param->id);

        int i = 6;
        switch (param->type) {
        case UINT8:
            send_msg_buffer[i++] = (u8)(u32)param->value;
            break;
        case INT8:
            send_msg_buffer[i++] = (s8)(u32)param->value;
            break;
        case UINT16:
            send_msg_buffer[i++] = (u16)(u32)param->value >> 8;
            send_msg_buffer[i++] = (u16)(u32)param->value;
            break;
        case INT16:
            send_msg_buffer[i++] = (s16)(u32)param->value >> 8;
            send_msg_buffer[i++] = (s16)(u32)param->value;
            break;
        case FLOAT:
            send_msg_buffer[i++] = (s32)param->value >> 24;
            send_msg_buffer[i++] = (s32)param->value >> 16;
            send_msg_buffer[i++] = (s32)param->value >> 8;
            send_msg_buffer[i++] = (s32)param->value;
            break;
        case TEXT_SELECTION:
            send_msg_buffer[i++] = (u8)param->u.text_sel;
            break;
        case STRING:
            i += strlcpy((char *)&send_msg_buffer[i], (char *)param->value, SEND_MSG_BUF_SIZE);
        case OUT_OF_RANGE:
        default:
            break;
        }

        send_msg_buffer[1] = i - 1;
        send_msg_buffer[i++] = crsf_crc8(&send_msg_buffer[2], send_msg_buffer[1]-1);
        send_msg_buf_count = i;
        read_timeout = CLOCK_getms();
    }
}

void CRSF_send_command(crsf_param_t *param, enum cmd_status status) {
    if (!send_msg_buf_count) {
        next_param = param->id;    // device responds with parameter info so prepare to receive
        next_chunk = 0;
        recv_param_ptr = recv_param_buffer;

        param_msg_header(TYPE_SETTINGS_WRITE, crsf_devices[param->device].address, param->id);
        send_msg_buffer[6] = status;
        send_msg_buffer[7] = crsf_crc8(&send_msg_buffer[2], send_msg_buffer[1]-1);
        send_msg_buf_count = 8;
        if (param->u.status != CONFIRMATION_NEEDED)
            command.time = CLOCK_getms();
    }
}

/**************************************************************************/
// Code in this section runs in serial port rx interrupt context
/**************************************************************************/
static u32 parse_u32(const u8 *buffer) {
    return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
}

static void parse_bytes(enum data_type type, char **buffer, void *dest) {
    switch (type) {
    case UINT8:
        *(u8 *)dest = (u8) (*buffer)[0];
        *buffer += 1;
        break;
    case INT8:
        *(s8 *)dest = (s8) (*buffer)[0];
        *buffer += 1;
        break;
    case UINT16:
        *(u16 *)dest = (u16) (((*buffer)[0] << 16) | (*buffer)[1]);
        *buffer += 2;
        break;
    case INT16:
        *(s16 *)dest = (s16) (((*buffer)[0] << 16) | (*buffer)[1]);
        *buffer += 2;
        break;
    case FLOAT:
        *(s32 *)dest = (s32) (((*buffer)[0] << 24) | ((*buffer)[1] << 16)
                     |        ((*buffer)[2] << 8)  |  (*buffer)[3]);
        *buffer += 4;
        break;
    default:
        break;
    }
}

char *alloc_string(u32 bytes) {
//    if ((next_string - recv_param_buffer) + bytes >= sizeof recv_param_buffer)
//        return NULL;
    char *p = next_string;
    next_string += bytes;
    return p;
}

static void add_device(u8 *buffer) {
    for (int i=0; i < CRSF_MAX_DEVICES; i++) {
        if (crsf_devices[i].address == buffer[2]) return;  //  device already in table
        if (crsf_devices[i].address == 0) {
            //  not found, add to table
            buffer += 2;
            crsf_devices[i].address = (u8) *buffer++;
            buffer += strlcpy(crsf_devices[i].name, (const char *)buffer, CRSF_MAX_NAME_LEN) + 1;
            crsf_devices[i].serial_number = parse_u32(buffer);
            buffer += 4;
            crsf_devices[i].hardware_id = parse_u32(buffer);
            buffer += 4;
            crsf_devices[i].firmware_id = parse_u32(buffer);
            buffer += 4;
            crsf_devices[i].number_of_params = *buffer;
            buffer += 1;
            crsf_devices[i].params_version = *buffer;
            break;
        }
    }
    //  no new device added if no more space in table
}

static void add_param(u8 *buffer, u8 num_bytes) {
    // abort if wrong device, or not enough buffer space
    if (buffer[2] != crsf_devices[device_idx].address
     || ((u8)((sizeof recv_param_buffer) - (recv_param_ptr - recv_param_buffer)) < (num_bytes-4))) {
        recv_param_ptr = recv_param_buffer;
        next_chunk = 0;
        next_param = 0;
        return;
    }

    memcpy(recv_param_ptr, buffer+5, num_bytes-5);
    recv_param_ptr += num_bytes - 5;

    if (buffer[4] > 0) {
        if (buffer[4] >= CRSF_MAX_CHUNKS) {
            recv_param_ptr = recv_param_buffer;
            next_chunk = 0;
            next_param = 0;
            return;
        } else {
            next_chunk += 1;
            CRSF_read_param(device_idx, next_param, next_chunk);
        }
        return;
    }

    // received all chunks for current parameter
    recv_param_ptr = recv_param_buffer;
    crsf_param_t *parameter = crsf_params;
    for (int i=0; i < CRSF_MAX_PARAMS; i++, parameter++) {
        int update = parameter->id == buffer[3];

        if (update || parameter->id == 0) {
            parameter->device = device_idx;
            parameter->id = buffer[3];
            parameter->parent = *recv_param_ptr++;
            parameter->type = *recv_param_ptr & 0x7f;
            if (!update) {
                parameter->hidden = *recv_param_ptr++ & 0x80;
                parameter->name = alloc_string(strlen(recv_param_ptr)+1);
                recv_param_ptr += strlcpy(parameter->name, (const char *)recv_param_ptr,
                                      CRSF_STRING_BYTES_AVAIL(parameter->name)) + 1;
            } else {
                if (parameter->hidden != (*recv_param_ptr & 0x80))
                    params_loaded = 0;   // if item becomes hidden others may also, so reload all params
                parameter->hidden = *recv_param_ptr++ & 0x80;
                recv_param_ptr += strlen(recv_param_ptr) + 1;
            }
            int count;
            switch (parameter->type) {
            case UINT8:
            case INT8:
            case UINT16:
            case INT16:
            case FLOAT:
                parse_bytes(parameter->type, &recv_param_ptr, &parameter->value);
                parse_bytes(parameter->type, &recv_param_ptr, &parameter->min_value);
                parse_bytes(parameter->type, &recv_param_ptr, &parameter->max_value);
                parse_bytes(parameter->type, &recv_param_ptr, &parameter->default_value);
                if (parameter->type == FLOAT) {
                    parse_bytes(UINT8, &recv_param_ptr, &parameter->u.point);
                    parse_bytes(FLOAT, &recv_param_ptr, &parameter->step);
                } else if (*recv_param_ptr) {
                    if (!update) parameter->s.unit = alloc_string(strlen(recv_param_ptr)+1);
                    strlcpy(parameter->s.unit, (const char *)recv_param_ptr,
                            CRSF_STRING_BYTES_AVAIL(parameter->s.unit));
                }
                break;

            case TEXT_SELECTION:
                if (!update) {
                    parameter->value = alloc_string(strlen(recv_param_ptr)+1);
                    recv_param_ptr += strlcpy(parameter->value,
                                         (const char *)recv_param_ptr,
                                         CRSF_STRING_BYTES_AVAIL(parameter->value)) + 1;
                    // put null between selection options
                    // find max choice string length to adjust textselectplate size
                    char *start = (char *)parameter->value;
                    int max_len = 0;
                    count = 0;
                    for (char *p = (char *)parameter->value; *p; p++) {
                        if (*p == ';') {
                            *p = '\0';
                            if (p - start > max_len) {
                                parameter->max_str = start;
                                max_len = p - start;
                            }
                            start = p+1;
                            count += 1;
                        }
                    }
                    parameter->max_value = count;   // bug fix for incorrect max from device
                } else {
                    recv_param_ptr += strlen(recv_param_ptr) + 1;
                }
                parse_bytes(UINT8, &recv_param_ptr, &parameter->u.text_sel);
                parse_bytes(UINT8, &recv_param_ptr, &parameter->min_value);
                parse_bytes(UINT8, &recv_param_ptr, &count);  // don't use incorrect parameter->max_value
                parse_bytes(UINT8, &recv_param_ptr, &parameter->default_value);
                break;

            case INFO:
                if (!update) {
                    parameter->value = alloc_string(strlen(recv_param_ptr)+1);
                    recv_param_ptr += strlcpy(parameter->value,
                                         (const char *)recv_param_ptr,
                                         CRSF_STRING_BYTES_AVAIL(parameter->value)) + 1;
                }
                break;

            case STRING:
                {
                    const char *value, *default_value;
                    value = recv_param_ptr;
                    recv_param_ptr += strlen(value) + 1;
                    default_value = recv_param_ptr;
                    recv_param_ptr += strlen(default_value) + 1;
                    parse_bytes(UINT8, &recv_param_ptr, &parameter->u.string_max_len);

                    // No string re-sizing so allocate max length for value
                    if (!update) parameter->value = alloc_string(parameter->u.string_max_len+1);
                    strlcpy(parameter->value, value,
                            MIN(parameter->u.string_max_len+1,
                                CRSF_STRING_BYTES_AVAIL(parameter->value)));
                    if (!update) parameter->default_value = alloc_string(strlen(default_value)+1);
                    strlcpy(parameter->default_value, default_value,
                            CRSF_STRING_BYTES_AVAIL(parameter->default_value));
                }
                break;

            case COMMAND:
                parse_bytes(UINT8, &recv_param_ptr, &parameter->u.status);
                parse_bytes(UINT8, &recv_param_ptr, &parameter->timeout);
                if (!update) parameter->s.info = alloc_string(20);
                strlcpy(parameter->s.info, (const char *)recv_param_ptr, 20);

                command.param = parameter;
                command.time = 0;
                switch (parameter->u.status) {
                case PROGRESS:
                    command.time = CLOCK_getms();
                    // FALLTHROUGH
                case CONFIRMATION_NEEDED:
                    command.dialog = 1;
                    break;
                case READY:
                    command.dialog = 2;
                    break;
                }
                break;

            case FOLDER:
            case OUT_OF_RANGE:
            default:
                break;
            }

            break;  // add or update completed
        }
    }

    recv_param_ptr = recv_param_buffer;
    next_chunk = 0;

    // read all params when needed
    if (params_loaded < crsf_devices[device_idx].number_of_params) {
        if (next_param < crsf_devices[device_idx].number_of_params)
            next_param += 1;
        else
            next_param = 1;
        CRSF_read_param(device_idx, next_param, next_chunk);
    } else {
        next_param = 0;

    // after all params loaded, schedule redraw if number of menu items changed
        int count = folder_rows(current_folder);
        if (count != current_folder_count) {
            command.redraw = 1;
        }
    }
}

// called from UART receive ISR when extended packet received
void CRSF_serial_rcv(u8 *buffer, u8 num_bytes) {
    switch (buffer[0]) {
    case TYPE_DEVICE_INFO:
        add_device(buffer);
        break;

    case TYPE_SETTINGS_ENTRY:
        read_timeout = 0;
        add_param(buffer, num_bytes);
        break;

    default:
        break;
    }
}

/**************************************************************************/
// Called from CRSF protocol callback to send data if available
// Copy send data to transmit buffer
// Return number of bytes to send. Must be complete CRSF packet.
u8 CRSF_serial_txd(u8 *buffer, u8 max_len) {
    u8 bytes_to_send = 0;
    if (send_msg_buf_count) {
        bytes_to_send = send_msg_buf_count < max_len ? send_msg_buf_count : max_len;
        memcpy(buffer, send_msg_buffer, bytes_to_send);
        send_msg_buf_count -= bytes_to_send;
    }
    return bytes_to_send;
}

#endif
