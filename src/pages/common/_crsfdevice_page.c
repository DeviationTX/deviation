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

#include "target/drivers/serial/usb_cdc/CBUF.h"
#include "crsf.h"

crsf_param_t crsf_params[CRSF_MAX_PARAMS];

static struct crsfdevice_page * const mp = &pagemem.u.crsfdevice_page;
static struct crsfdevice_obj * const gui = &gui_objs.u.crsfdevice;

static u32 read_timeout;
static u8 current_folder = 255;
static u8 need_show_folder = 255;
static volatile u8 params_loaded;     // if not zero, number received so far for current device
static volatile u8 params_displayed;  // if not zero, number displayed so far for current device
static u8 device_idx;   // current device index
static u8 next_param;   // parameter and chunk currently being read
static u8 next_chunk;

static struct {
    crsf_param_t *param;
    u32 time;
    u16 timeout;
    u8  dialog;
} command;

static struct {
    volatile uint8_t    m_get_idx;
    volatile uint8_t    m_put_idx;
    uint8_t             m_entry[128];  // must be power of 2
} send_buf;

#define CRSF_MAX_CHUNK_SIZE   58   // 64 - header - type - destination - origin
#define CRSF_MAX_CHUNKS        8   // not in specification. Max observed is 5 for ELRS 2.3 (HappyModel ES2400TX)
static char recv_param_buffer[CRSF_MAX_CHUNKS * CRSF_MAX_CHUNK_SIZE];
static char *recv_param_ptr;

#define SEND_MSG_BUF_SIZE  64      // don't send more than one chunk

static char *next_string;

#define MIN(a, b) ((a) < (b) ? a : b)

static void crsfdevice_init() {
    next_param = 1;
    next_chunk = 0;
    recv_param_ptr = recv_param_buffer;
    params_loaded = 0;
    params_displayed = 0;
    next_string = mp->strings;
    memset(crsf_params, 0, sizeof crsf_params);
    CBUF_Init(send_buf);
}

crsf_param_t *current_param(int absrow) {
    int idx = 0;

    for (int i=0; i < crsf_devices[device_idx].number_of_params; i++) {
        if (!crsf_params[i].id) break;
        if (crsf_params[i].parent != current_folder || crsf_params[i].hidden) continue;
        if (idx++ == absrow) return &crsf_params[i];
    }
    return NULL;
}

static const char *current_text(crsf_param_t *param) {
    const char *p = (const char *)param->value;
    int i = param->u.text_sel;
    while (i--) while (*p++) {}
    return p;
}

static const char *crsf_value_cb(guiObject_t *obj, const void *data)
{
    (void)obj;

    crsf_param_t *param = (crsf_param_t *)data;

    switch (param->type) {
    case UINT8:
    case UINT16:
    case FLOAT:
        snprintf(tempstring, sizeof tempstring, "%d", (uintptr_t)param->value);
        break;
    case INT8:
    case INT16:
        snprintf(tempstring, sizeof tempstring, "%d", (intptr_t)param->value);
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
        snprintf(tempstring, sizeof tempstring, "%s   %s", param->name, param->value);
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

static const char *crsf_name_cb(guiObject_t *obj, const void *data)
{
    (void)obj;

    crsf_param_t *param = (crsf_param_t *)data;

    if (param->id == 0) return "--";
    if (param->type == INFO)
        return crsf_value_cb(obj, data);

    return (const char *)param->name;
}

static u8 count_params_loaded() {
    u8 count = crsf_params[0].loaded;
    for (int i=1; i < crsf_devices[device_idx].number_of_params; i++)
        count += crsf_params[i].loaded;
    return count;
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
    CRSF_set_param(param);
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

    if (changed) {
        param->changed = 1;
        CRSF_set_param(param);
    }
    return current_text(param);
}

static const char *value_numsel(guiObject_t *obj, int dir, void *data)
{
    (void)obj;
    crsf_param_t *param = (crsf_param_t *)data;
    u8 changed = 0;

    param->value = (void *)(intptr_t)GUI_TextSelectHelper((intptr_t)param->value,
                                        param->min_value, param->max_value,
                                        dir, param->step, 10*param->step, &changed);

    if (changed) {
        param->changed = 1;
        CRSF_set_param(param);
    }

    snprintf(tempstring, sizeof tempstring, "%d", (intptr_t)param->value);
    if (param->type == FLOAT && param->u.point > 0) {
        int pos = strlen(tempstring) - param->u.point;
        memmove(&tempstring[pos+1], &tempstring[pos], param->u.point+1);
        tempstring[pos] = '.';
    }
    return tempstring;
}

static void show_page(u8 folder);
static void show_header();

static u8 param_next(u8 param) {
    int break_count = 0;
    if (param == 0) param = 1;  // exception for root folder
    while (crsf_params[param-1].loaded) {
        if (param < crsf_devices[device_idx].number_of_params)
            param += 1;
        else
            param = 1;
        if (break_count++ >= crsf_devices[device_idx].number_of_params)
            return 0;
    }
    return param;
}

static void folder_load(u8 param_id) {
    // param_id of 0 indicates root folder
    if (param_id != 0) {
        crsf_params[param_id-1].loaded = 0;
        params_loaded -= 1;
        need_show_folder = param_id;
    }
    if (param_id == 0 || crsf_params[param_id-1].type == FOLDER) {
        for (int i=0; i < crsf_devices[device_idx].number_of_params; i++) {
            if (crsf_params[i].parent == param_id) {
                crsf_params[i].loaded = 0;
                params_loaded -= 1;
                need_show_folder = param_id;
            }
        }
    }
    next_string = mp->strings;      // re-allocate all strings
    next_param = param_next(param_id);
    next_chunk = 0;
    CRSF_read_param(device_idx, next_param, next_chunk);
}

static void folder_cb(struct guiObject *obj, s8 press_type, const void *data)
{
    (void)obj;
    if (press_type != -1) {
        return;
    }

    crsf_param_t *param = (crsf_param_t *)data;

    folder_load(param->id);
}

static void noop_press(struct guiObject *obj, s8 press_type, const void *data)
{
    (void)obj;
    (void)press_type;
    (void)data;
    return;
}

static void stredit_done_cb(guiObject_t *obj, void *data)  // devo8 doesn't handle cancel/discard properly,
{
    crsf_param_t *param = (crsf_param_t *)data;

    GUI_RemoveObj(obj);
    if (param) {  // Keyboard sets to null if changes discarded
        strlcpy((char *)param->value, (const char *)tempstring, param->u.string_max_len);
        param->changed = 1;
        CRSF_set_param(param);
    }
    folder_load(current_folder);
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

static crsf_param_t *param_by_id(int id) {
    crsf_param_t *param = crsf_params;

    while (param->id) {
        if (param->id == id)
            return param;
        param++;
    }
    return NULL;
}

// Handle buttons
static unsigned action_cb(u32 button, unsigned flags, void *data)
{
    if (((flags & BUTTON_PRESS) && !(flags & BUTTON_LONGPRESS))
     && (CHAN_ButtonIsPressed(button, BUT_EXIT)
        || CHAN_ButtonIsPressed(button, BUT_DOWN)
        || CHAN_ButtonIsPressed(button, BUT_UP)))
    {
        int idx = GUI_ScrollableGetObjRowOffset(&gui->scrollable, GUI_GetSelected());
        int absrow = (idx >> 8) + (idx & 0xff);
        crsf_param_t *param = current_param(absrow);

        if (param->type <= STRING && param->changed) {
            param->changed = 0;

            // ELRS does not echo parameters that are changed, or other parameters
            // that change as a consequence of a different parameter being changed.
            // As a workaround, reload folder on every update.
            folder_load(param->parent);
        }
    }

    if (current_folder != 0 && CHAN_ButtonIsPressed(button, BUT_EXIT)) {
        if (flags & BUTTON_RELEASE) {
            folder_load(param_by_id(current_folder)->parent);
        }
        return 1;
    }
    return default_button_action_cb(button, flags, data);
}

void PAGE_CRSFDeviceEvent() {
    // update page as parameter info is received
    // until all parameters loaded
    static u8 armed_state;
    u8 params_count = count_params_loaded();
    if (params_displayed != params_count) {
        params_displayed = params_count;
        show_page(current_folder);
    } else if (elrs_info.update > 0 || armed_state != protocol_elrs_is_armed()) {
        elrs_info.update = 0;
        armed_state = protocol_elrs_is_armed();
        show_header();
    }

    // commands may require interaction through dialog
    if (command.dialog == 1) {
        command.dialog = 0;
        PAGE_CRSFdialog((void *)command.param);
    } else if (command.dialog == 2) {
        command.dialog = 0;
        PAGE_CRSFdialogClose();
    }
    if (command.time && (CLOCK_getms() - command.time > command.param->timeout * 100)) {
        if (command.param->u.status != READY) {
            CRSF_send_command(command.param, POLL);
            command.time = CLOCK_getms();
            return;
        } else {
            PAGE_CRSFdialogClose();
            command.time = 0;
        }
    }

    // spec calls for 2 second timeout on requests. Retry on timeout.
    if (read_timeout && (CLOCK_getms() - read_timeout > 2000)) {
        if (next_param) CRSF_read_param(device_idx, next_param, next_chunk);
        else read_timeout = 0;
    }
}

// Following functions queue a CRSF message for sending

// start with crc=0
static void buf_push_crc(u8 *crc, u8 val) {
    CBUF_Push(send_buf, val);
    crsf_crc8_acc(crc, val);
}
static void buf_push_crc2(u8 *crc, u8 *crc_BA, u8 val) {
    CBUF_Push(send_buf, val);
    crsf_crc8_acc(crc, val);
    crsf_crc8_BA_acc(crc_BA, val);
}

void CRSF_ping_devices(u8 address) {
    if (CBUF_Space(send_buf) < 6) return;
    u8 crc = 0;
    CBUF_Push(send_buf, ADDR_MODULE);
    CBUF_Push(send_buf, 4);
    buf_push_crc(&crc, TYPE_PING_DEVICES);
    buf_push_crc(&crc, address);
    buf_push_crc(&crc, ADDR_RADIO);
    CBUF_Push(send_buf, crc);
}

u8 CRSF_send_model_id(u8 model_id) {
    if (CBUF_Space(send_buf) < 10) return 0;
    u8 crc = 0;
    u8 crc_BA = 0;
    CBUF_Push(send_buf, ADDR_MODULE);
    CBUF_Push(send_buf, 8);
    buf_push_crc2(&crc, &crc_BA, TYPE_COMMAND_ID);
    buf_push_crc2(&crc, &crc_BA, ADDR_MODULE);
    buf_push_crc2(&crc, &crc_BA, ADDR_RADIO);
    buf_push_crc2(&crc, &crc_BA, CRSF_SUBCOMMAND);
    buf_push_crc2(&crc, &crc_BA, COMMAND_MODEL_SELECT_ID);
    buf_push_crc2(&crc, &crc_BA, model_id);
    buf_push_crc(&crc, crc_BA);
    CBUF_Push(send_buf, crc);
    return 1;
}

u8 CRSF_speed_proposal(u32 bitrate) {
    if (CBUF_Space(send_buf) < 14) return 0;
    u8 crc = 0;
    u8 crc_BA = 0;
    CBUF_Push(send_buf, ADDR_MODULE);
    CBUF_Push(send_buf, 12);
    buf_push_crc2(&crc, &crc_BA, TYPE_COMMAND_ID);
    buf_push_crc2(&crc, &crc_BA, ADDR_MODULE);
    buf_push_crc2(&crc, &crc_BA, ADDR_RADIO);
    buf_push_crc2(&crc, &crc_BA, GENERAL_SUBCMD);
    buf_push_crc2(&crc, &crc_BA, SUBCMD_SPD_PROPOSAL);
    buf_push_crc2(&crc, &crc_BA, 0);
    buf_push_crc2(&crc, &crc_BA, bitrate >> 24);
    buf_push_crc2(&crc, &crc_BA, bitrate >> 16);
    buf_push_crc2(&crc, &crc_BA, bitrate >> 8);
    buf_push_crc2(&crc, &crc_BA, bitrate);
    buf_push_crc(&crc, crc_BA);
    CBUF_Push(send_buf, crc);
    return 1;
}

u8 CRSF_speed_response(u8 accept, usart_callback_t tx_callback) {
    if (CBUF_Space(send_buf) < 11) return 0;
    if (tx_callback) UART_TxCallback(tx_callback);
    u8 crc = 0;
    u8 crc_BA = 0;
    CBUF_Push(send_buf, ADDR_MODULE);
    CBUF_Push(send_buf, 9);
    buf_push_crc2(&crc, &crc_BA, TYPE_COMMAND_ID);
    buf_push_crc2(&crc, &crc_BA, ADDR_MODULE);
    buf_push_crc2(&crc, &crc_BA, ADDR_RADIO);
    buf_push_crc2(&crc, &crc_BA, GENERAL_SUBCMD);
    buf_push_crc2(&crc, &crc_BA, SUBCMD_SPD_RESPONSE);
    buf_push_crc2(&crc, &crc_BA, 0);
    buf_push_crc2(&crc, &crc_BA, accept ? 1 : 0);
    buf_push_crc(&crc, crc_BA);
    CBUF_Push(send_buf, crc);
    return 1;
}

u8 CRSF_command_ack(u8 cmd_id, u8 sub_cmd_id, u8 action) {
    if (CBUF_Space(send_buf) < 12) return 0;
    u8 crc = 0;
    u8 crc_BA = 0;
    CBUF_Push(send_buf, ADDR_MODULE);
    CBUF_Push(send_buf, 10);
    buf_push_crc2(&crc, &crc_BA, TYPE_COMMAND_ID);
    buf_push_crc2(&crc, &crc_BA, ADDR_MODULE);
    buf_push_crc2(&crc, &crc_BA, ADDR_RADIO);
    buf_push_crc2(&crc, &crc_BA, ACK_SUBCMD);
    buf_push_crc2(&crc, &crc_BA, cmd_id);
    buf_push_crc2(&crc, &crc_BA, sub_cmd_id);
    buf_push_crc2(&crc, &crc_BA, action ? 1 : 0);
    buf_push_crc2(&crc, &crc_BA, 0);     // optional null terminated string
    buf_push_crc(&crc, crc_BA);
    CBUF_Push(send_buf, crc);
    return 1;
}

// Request parameter info from known device
void CRSF_read_param(u8 device, u8 id, u8 chunk) {
    if (CBUF_Space(send_buf) < 8 || id == 0) return;
    u8 crc = 0;
    CBUF_Push(send_buf, ADDR_MODULE);
    CBUF_Push(send_buf, 6);
    buf_push_crc(&crc, TYPE_SETTINGS_READ);
    buf_push_crc(&crc, crsf_devices[device].address);
    buf_push_crc(&crc, ADDR_RADIO);
    buf_push_crc(&crc, id);
    buf_push_crc(&crc, chunk);
    CBUF_Push(send_buf, crc);
    read_timeout = CLOCK_getms();
}

void CRSF_get_elrs() {
    // request ELRS_info message
    if (CBUF_Space(send_buf) < 8) return;
    u8 crc = 0;
    CBUF_Push(send_buf, ADDR_MODULE);
    CBUF_Push(send_buf, 6);
    buf_push_crc(&crc, TYPE_SETTINGS_WRITE);
    buf_push_crc(&crc, ADDR_MODULE);
    buf_push_crc(&crc, ADDR_RADIO);
    buf_push_crc(&crc, 0);
    buf_push_crc(&crc, 0);
    CBUF_Push(send_buf, crc);
}

static u8 param_len(crsf_param_t *param) {
        switch (param->type) {
        case UINT8:
        case INT8:
        case TEXT_SELECTION:
            return 1;
        case UINT16:
        case INT16:
            return 2;
        case FLOAT:
            return 4;
        case STRING:
            return strlen(param->value) + 1;
        case OUT_OF_RANGE:
        default:
            break;
        }
        return 0;
}

void CRSF_set_param(crsf_param_t *param) {
    if (crsf_devices[param->device].address == ADDR_RADIO) {
        protocol_set_param((u8)param->u.text_sel);  // only one radio param so don't need id
        return;
    }

    u16 length = param_len(param) + 5;
    if ((u16)CBUF_Space(send_buf) < length+2) return;

    u8 crc = 0;
    CBUF_Push(send_buf, ADDR_MODULE);
    CBUF_Push(send_buf, length);
    buf_push_crc(&crc, TYPE_SETTINGS_WRITE);
    buf_push_crc(&crc, crsf_devices[param->device].address);
    buf_push_crc(&crc, ADDR_RADIO);
    buf_push_crc(&crc, param->id);

    switch (param->type) {
    case UINT8:
        buf_push_crc(&crc, (u8)(uintptr_t)param->value);
        break;
    case INT8:
        buf_push_crc(&crc, (s8)(intptr_t)param->value);
        break;
    case UINT16:
        buf_push_crc(&crc, (u16)(uintptr_t)param->value >> 8);
        buf_push_crc(&crc, (u16)(uintptr_t)param->value);
        break;
    case INT16:
        buf_push_crc(&crc, (s16)(intptr_t)param->value >> 8);
        buf_push_crc(&crc, (s16)(intptr_t)param->value);
        break;
    case FLOAT:
        buf_push_crc(&crc, (intptr_t)param->value >> 24);
        buf_push_crc(&crc, (intptr_t)param->value >> 16);
        buf_push_crc(&crc, (intptr_t)param->value >> 8);
        buf_push_crc(&crc, (intptr_t)param->value);
        break;
    case TEXT_SELECTION:
        buf_push_crc(&crc, (u8)param->u.text_sel);
        break;
    case STRING:
        {  char *p = (char*)param->value;
        for (size_t i=0; i <= strlen((char *)param->value); i++)
            buf_push_crc(&crc, *p++);
        }
        break;
    case OUT_OF_RANGE:
    default:
        break;
    }
    CBUF_Push(send_buf, crc);
}

void CRSF_send_command(crsf_param_t *param, enum cmd_status status) {
    if (CBUF_Space(send_buf) < 8) return;

    next_param = param->id;    // device responds with parameter info so prepare to receive
    next_chunk = 0;
    recv_param_ptr = recv_param_buffer;

    u8 crc = 0;
    CBUF_Push(send_buf, ADDR_MODULE);
    CBUF_Push(send_buf, 6);
    buf_push_crc(&crc, TYPE_SETTINGS_WRITE);
    buf_push_crc(&crc, crsf_devices[param->device].address);
    buf_push_crc(&crc, ADDR_RADIO);
    buf_push_crc(&crc, param->id);
    buf_push_crc(&crc, status);
    CBUF_Push(send_buf, crc);

    if (param->u.status != CONFIRMATION_NEEDED)
        command.time = CLOCK_getms();
}

/**************************************************************************/
// Called from CRSF protocol callback to send data if available
// Copy send data to transmit buffer. Buffer must be large enough (64 bytes)
// Return number of bytes to send. Must be complete CRSF packet.
u8 CRSF_serial_txd(u8 *buffer) {
    u8 len = CBUF_Len(send_buf);
    if (len < 2) return 0;

    u8 pkt_size = CBUF_Get(send_buf, 1) + 2;
    if (len >= pkt_size) {
        for (int i=0; i < pkt_size; i++)
            buffer[i] = CBUF_Pop(send_buf);
        return pkt_size;
    }
    return 0;
}
/**************************************************************************/

/*****************************************************************************************/
// Code in this section runs in CLOCK_RunOnce interrupt context (software initiated EXTI3)
/*****************************************************************************************/
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
        *(u16 *)dest = (u16) (((*buffer)[0] << 8) | (*buffer)[1]);
        *buffer += 2;
        break;
    case INT16:
        *(s16 *)dest = (s16) (((*buffer)[0] << 8) | (*buffer)[1]);
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

static char *alloc_string(s32 bytes) {
#ifdef EMULATOR
  return calloc(1, bytes);
#endif
    if (CRSF_STRING_BYTES_AVAIL(next_string) < bytes)
        return NULL;

    char *p = next_string;
    next_string += bytes;
    return p;
}

static void parse_device(u8* buffer, crsf_device_t *device) {
    buffer += 2;
    device->address = (u8) *buffer++;
    strlcpy(device->name, (const char *)buffer, CRSF_MAX_NAME_LEN);
    buffer += strlen((const char*)buffer) + 1;
    device->serial_number = parse_u32(buffer);
    buffer += 4;
    device->hardware_id = parse_u32(buffer);
    buffer += 4;
    device->firmware_id = parse_u32(buffer);
    buffer += 4;
    device->number_of_params = *buffer;
    buffer += 1;
    device->params_version = *buffer;
    if (device->address == ADDR_MODULE) {
        if (device->serial_number == 0x454C5253)
            protocol_module_type(MODULE_ELRS);
        else
            protocol_module_type(MODULE_OTHER);
    }
}

static void add_device(u8 *buffer) {
    for (int i=0; i < CRSF_MAX_DEVICES; i++) {
        if (crsf_devices[i].address == buffer[2]        //  device already in table
         || crsf_devices[i].address == 0                //  not found, add to table
         || crsf_devices[i].address == ADDR_RADIO) {    //  replace deviation device if necessary
            parse_device(buffer, &crsf_devices[i]);
            break;
        }
    }
    //  no new device added if no more space in table
}

/*  ELRS flag meanings as of 17 March 2022
enum lua_Flags{
      //bit 0 and 1 are status flags, show up as the little icon in the lua top right corner
      LUA_FLAG_CONNECTED = 0,
      LUA_FLAG_STATUS1,
      //bit 2,3,4 are warning flags, change the tittle bar every 0.5s
      LUA_FLAG_MODEL_MATCH,
      LUA_FLAG_ISARMED,
      LUA_FLAG_WARNING1,
      //bit 5,6,7 are critical warning flag, block the lua screen until user confirm to suppress the warning.
      LUA_FLAG_ERROR_CONNECTED,
      LUA_FLAG_CRITICAL_WARNING1,
      LUA_FLAG_CRITICAL_WARNING2,
  };
*/
static void parse_elrs_info(u8 *buffer) {
    elrs_info_t local_info;

    local_info.bad_pkts = buffer[3];                      // bad packet rate (should be 0)
    local_info.good_pkts = (buffer[4] << 8) + buffer[5];  // good packet rate (configured rate)
    // flags bit 0 indicates receiver connected
    // other bits indicate errors - error text in flags_info
    local_info.flags = buffer[6];
    strlcpy(local_info.flag_info, (const char*)&buffer[7], CRSF_MAX_NAME_LEN);  // null-terminated text of flags

    // save in global for use in UI
    local_info.update = elrs_info.update;
    if (memcmp((void*)&elrs_info, (void*)&local_info, sizeof(elrs_info_t)-CRSF_MAX_NAME_LEN)) {
        if (local_info.flag_info[0] && strncmp(local_info.flag_info, elrs_info.flag_info, CRSF_MAX_NAME_LEN)) {
            if (local_info.flags & 0x4)
                PAGE_ShowWarning(NULL, local_info.flag_info);       // show warning dialog if model mismatch
            MUSIC_Beep("d2", 100, 100, 5);
        }

        memcpy((void*)&elrs_info, (void*)&local_info, sizeof(elrs_info_t));
        elrs_info.update += 1;
    }
}

static void add_param(u8 *buffer, u8 num_bytes) {
    u8 length;

    // abort if wrong device, or not enough buffer space
    if (buffer[2] != crsf_devices[device_idx].address
     || ((int)((sizeof recv_param_buffer) - (recv_param_ptr - recv_param_buffer)) < (num_bytes-4))) {
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
    // all devices so far start parameter id at 1...fingers crossed
    if (buffer[3] >= CRSF_MAX_PARAMS) {
        next_chunk = 0;
        next_param = 0;
        return;
    }
    crsf_param_t *parameter = &crsf_params[buffer[3]-1];

    parameter->device = device_idx;
    parameter->id = buffer[3];
    parameter->parent = *recv_param_ptr++;
    parameter->type = *recv_param_ptr & 0x7f;
    parameter->loaded = 1;
    parameter->hidden = *recv_param_ptr++ & 0x80;
    // reallocate name string
    parameter->name_size = strlen(recv_param_ptr) + 1;
    parameter->name = alloc_string(parameter->name_size);
    strlcpy(parameter->name, (const char *)recv_param_ptr, parameter->name_size);
    recv_param_ptr += parameter->name_size;

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
        } else {
            parameter->step = parameter->min_value != parameter->max_value;
            if (*recv_param_ptr) {
                length = strlen(recv_param_ptr) + 1;
                parameter->s.unit = alloc_string(length);
                strlcpy(parameter->s.unit, (const char *)recv_param_ptr, length);
            }
        }
        break;

    case TEXT_SELECTION:
        length = strlen(recv_param_ptr) + 1;
        parameter->value = alloc_string(length);
        strlcpy(parameter->value, (const char *)recv_param_ptr, length);
        recv_param_ptr += length;
        // put null between selection options
        // find max choice string length to adjust textselectplate size
        char *start = (char *)parameter->value;
        int max_len = 0;
        count = 0;
        for (char *p = (char *)parameter->value; *p; p++) {
            if (*p == ';') {
                *p = '\0';
                if (p - start > max_len) {
                    parameter->max_str = start;  // save max to determine gui box size
                    max_len = p - start;
                }
                start = p+1;
                count += 1;
            }
            // handle "Lua up arrow and down arrow - replace with u and d
            else if (*p == 0xc0) *p = 'u';
            else if (*p == 0xc1) *p = 'd';
            else if (*p &  0x80) *p = '?';
        }
        parameter->max_value = count;   // bug fix for incorrect max from device
        parse_bytes(UINT8, &recv_param_ptr, &parameter->u.text_sel);
        parse_bytes(UINT8, &recv_param_ptr, &parameter->min_value);
        parse_bytes(UINT8, &recv_param_ptr, &count);  // don't use incorrect parameter->max_value
        parse_bytes(UINT8, &recv_param_ptr, &parameter->default_value);
        break;

    case INFO:
        length = strlen(recv_param_ptr) + 1;
        parameter->value = alloc_string(length);
        strlcpy(parameter->value, (const char *)recv_param_ptr, length);
        break;

    case STRING:
        {
            const char *value;
            value = recv_param_ptr;
            recv_param_ptr += strlen(value) + 1;
            parse_bytes(UINT8, &recv_param_ptr, &parameter->u.string_max_len);

            parameter->value = alloc_string(parameter->u.string_max_len+1);
            strlcpy(parameter->value, value, parameter->u.string_max_len+1);
        }
        break;

    case COMMAND:
        parse_bytes(UINT8, &recv_param_ptr, &parameter->u.status);
        parse_bytes(UINT8, &recv_param_ptr, &parameter->timeout);
        parameter->s.info = alloc_string(40);
        strlcpy(parameter->s.info, (const char *)recv_param_ptr, 40);

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

    recv_param_ptr = recv_param_buffer;
    next_chunk = 0;
    params_loaded = count_params_loaded();

    // read params until all loaded
    next_param = param_next(next_param);
    if (next_param > 0) CRSF_read_param(device_idx, next_param, next_chunk);
}

// called from UART receive ISR when extended packet received
void CRSF_serial_rcv(u8 *buffer, u8 num_bytes) {
    switch (buffer[0]) {
    case TYPE_DEVICE_INFO:
        add_device(buffer);
        break;

    case TYPE_ELRS_INFO:
        parse_elrs_info(buffer);
        break;

    case TYPE_SETTINGS_ENTRY:
        read_timeout = 0;
        add_param(buffer, num_bytes);
        break;

    default:
        break;
    }
}

#endif
