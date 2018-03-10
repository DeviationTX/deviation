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


#ifdef MODULAR
  //Allows the linker to properly relocate
  #define CORONA_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h"

#ifdef MODULAR
  //Some versions of gcc applythis to definitions, others to calls
  //So just use long_calls everywhere
  //#pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#ifdef PROTO_HAS_CC2500

#include "iface_cc2500.h"

static const char *const corona_opts[] = {
    _tr_noop("Format"), "V1", "V2", "FDV3", NULL,
    _tr_noop("Freq-Fine"),  "-127", "127", NULL,
    NULL
};

enum {
    PROTO_OPTS_FORMAT = 0,
    PROTO_OPTS_FREQFINE,
    LAST_PROTO_OPT,
};

enum {
    FORMAT_V1,
    FORMAT_V2,
    FORMAT_FDV3,
};
ctassert(LAST_PROTO_OPT <= NUM_PROTO_OPTS, too_many_protocol_opts);


//#define CORONA_FORCE_ID

#define CORONA_RF_NUM_CHANNELS    3
#define CORONA_ADDRESS_LENGTH     4
#define CORONA_BIND_CHANNEL_V1    0xD1  // also Flydream V3
#define CORONA_BIND_CHANNEL_V2    0xB8
#define CORONA_CHANNEL_TIMING     1500
#define FDV3_BIND_PERIOD          5000
#define FDV3_CHANNEL_PERIOD       4000

static u8  rx_tx_addr[CORONA_ADDRESS_LENGTH];
static u8  hopping_frequency[CORONA_RF_NUM_CHANNELS+1];
static u8  hopping_frequency_no;
static u16 bind_counter;
static u8  packet[40];
static u16 state;
static s8  fine;
static u8  fdv3_id_send;

// V1 radio init also same for Flydream V3
const u8 CORONA_init_values[] = {
  /* 00 */ 0x29, 0x2E, 0x06, 0x07, 0xD3, 0x91, 0xFF, 0x04,
  /* 08 */ 0x05, 0x00, CORONA_BIND_CHANNEL_V1, 0x06, 0x00, 0x5C, 0x4E, 0xC4,
  /* 10 */ 0x5B, 0xF8, 0x03, 0x23, 0xF8, 0x47, 0x07, 0x30,
  /* 18 */ 0x18, 0x16, 0x6C, 0x43, 0x40, 0x91, 0x87, 0x6B,
  /* 20 */ 0xF8, 0x56, 0x10, 0xA9, 0x0A, 0x00, 0x11, 0x41,
  /* 28 */ 0x00, 0x59, 0x7F, 0x3F, 0x81, 0x35, 0x0B
};

static void CORONA_rf_init() {

  CC2500_Strobe(CC2500_SIDLE);

  for (u8 i = 0; i <= 0x2E; ++i) CC2500_WriteReg(i, CORONA_init_values[i]);
  CC2500_Strobe(CC2500_SCAL);           // just duplicating stock tx
  CC2500_ReadReg(CC2500_25_FSCAL1);     // just duplicating stock tx

  if (Model.proto_opts[PROTO_OPTS_FORMAT] == FORMAT_V2) {
    CC2500_WriteReg(CC2500_0A_CHANNR, CORONA_BIND_CHANNEL_V2);
    CC2500_WriteReg(CC2500_0E_FREQ1, 0x80);
    CC2500_WriteReg(CC2500_0F_FREQ0, 0x00);
    CC2500_WriteReg(CC2500_15_DEVIATN, 0x50);
    CC2500_WriteReg(CC2500_17_MCSM1, 0x00);
    CC2500_WriteReg(CC2500_1B_AGCCTRL2, 0x67);
    CC2500_WriteReg(CC2500_1C_AGCCTRL1, 0xFB);
    CC2500_WriteReg(CC2500_1D_AGCCTRL0, 0xDC);
  }
  
  CC2500_WriteReg(CC2500_0C_FSCTRL0, Model.proto_opts[PROTO_OPTS_FREQFINE]);

  //not sure what they are doing to the PATABLE since basically only the first byte is used and it's only 8 bytes long. So I think they end up filling the PATABLE fully with 0xFF
//TODO  CC2500_WriteRegisterMulti(CC2500_3E_PATABLE,(const u8 *)"\x08\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 13);

  CC2500_SetTxRxMode(TX_EN);
  CC2500_SetPower(0);   // min power for binding, set in build_packet for normal operation
}

// Generate address to use from TX id and manufacturer id (STM32 unique id)
static void initialize_rx_tx_addr()
{
    u32 lfsr = 0xb2c54a2ful;

#ifndef USE_FIXED_MFGID
    u8 var[12];
    MCU_SerialNumber(var, 12);
    for (int i = 0; i < 12; ++i) {
        rand32_r(&lfsr, var[i]);
    }
#endif

    if (Model.fixed_id) {
       for (u8 i = 0, j = 0; i < sizeof(Model.fixed_id); ++i, j += 8)
           rand32_r(&lfsr, (Model.fixed_id >> j) & 0xff);
    }
    // Pump zero bytes for LFSR to diverge more
    for (u8 i = 0; i < sizeof (lfsr); ++i) rand32_r(&lfsr, 0);

    for (u8 i = 0; i < sizeof (rx_tx_addr); ++i) {
        rx_tx_addr[i] = lfsr & 0xff;
        rand32_r(&lfsr, i);
    }
}

// Generate id and hopping freq
static void CORONA_init()
{
//TODO #ifdef CORONA_FORCE_ID
if (!Model.fixed_id) {
    // Example of ID and channels taken from dumps
    switch (Model.proto_opts[PROTO_OPTS_FORMAT]) {
    case FORMAT_V1:
      memcpy((void *)rx_tx_addr,(void *)"\x1F\xFE\x6C\x35",CORONA_ADDRESS_LENGTH);
      memcpy((void *)hopping_frequency,(void *)"\x17\x0D\x03\x49",CORONA_RF_NUM_CHANNELS+1);
      break;
    case FORMAT_V2:
      memcpy((void *)rx_tx_addr,(void *)"\xFE\xFE\x02\xFB",CORONA_ADDRESS_LENGTH);
      memcpy((void *)hopping_frequency,(void *)"\x14\x3D\x35",CORONA_RF_NUM_CHANNELS);
      break;
    case FORMAT_FDV3:
      memcpy((void *)rx_tx_addr,(void *)"\x02\xfa\x38\x38",CORONA_ADDRESS_LENGTH);
      memcpy((void *)hopping_frequency,(void *)"\x71\xb9\x30",CORONA_RF_NUM_CHANNELS);
      break;
    }
//TODO #else
} else {
    // From dumps channels are anything between 0x00 and 0xC5 on V1.
    // But 0x00 and 0xB8 should be avoided on V2 since they are used for bind.
    // Below code make sure channels are between 0x02 and 0xA0, spaced with
    // a minimum of 2 and not ordered (RX only use the 1st channel unless there is an issue).
    initialize_rx_tx_addr();
    u8 order = rx_tx_addr[3] & 0x03;
    for (u8 i=0; i < CORONA_RF_NUM_CHANNELS+1; i++)
        hopping_frequency[i^order] = 2+rx_tx_addr[3-i]%39+(i<<5)+(i<<3);

    if (Model.proto_opts[PROTO_OPTS_FORMAT] != FORMAT_FDV3) {
        // ID looks random but on the 15 V1 dumps they all show the same odd/even rule
        if (rx_tx_addr[3] & 0x01) { // If [3] is odd then [0] is odd and [2] is even 
            rx_tx_addr[0] |= 0x01;
            rx_tx_addr[2] &= 0xFE;
        } else {                    // If [3] is even then [0] is even and [2] is odd 
            rx_tx_addr[0] &= 0xFE;
            rx_tx_addr[2] |= 0x01;
        }
        rx_tx_addr[1] = 0xFE;       // Always FE in the dumps of V1 and V2
    } else {
        rx_tx_addr[1] = 0xFA;       // Always FA for Flydream V3
        if (rx_tx_addr[3] > 0xa0) rx_tx_addr[3] &= 0x7f; // TODO thought this was id/freq channel but may not be
    }
}
}

u16 convert_channel_ppm(u8 chan) {
    return (u16)((s32)Channels[chan] * 500 / CHAN_MAX_VALUE + 1500);
}

static u16 CORONA_build_bind_pkt(void) {
  // Tune frequency if it has been changed
  if (fine != (s8)Model.proto_opts[PROTO_OPTS_FREQFINE]) {
      fine = (s8)Model.proto_opts[PROTO_OPTS_FREQFINE];
      CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);
  }

    // Build bind packets
    if (Model.proto_opts[PROTO_OPTS_FORMAT] == FORMAT_V1) {
      if (bind_counter&1) { // Send TX ID
        packet[0] = 0x04;    // 5 bytes to follow
        for(u8 i = 0; i < CORONA_ADDRESS_LENGTH; i++)
          packet[i+1] = rx_tx_addr[i];
        packet[5] = 0xCD;    // Unknown but seems to be always the same value for V1
        return 3689;
      } else {             // Send hopping freq
        packet[0] = 0x03;    // 4 bytes to follow
        for(u8 i = 0; i < CORONA_RF_NUM_CHANNELS+1; i++)
          packet[i+1] = hopping_frequency[i];
        // Not sure what the last byte (+1) is for now since only the first 3 channels are used...
        return 3438;
      }
    } else { // V2 and FDV3
      packet[0] = 0x04;   // 5 bytes to follow
      for(u8 i=0; i < CORONA_ADDRESS_LENGTH; i++)
        packet[i+1] = rx_tx_addr[i];
      packet[5] = 0x00;   // Unknown but seems to be always the same value for V2 and FDV3
      if (Model.proto_opts[PROTO_OPTS_FORMAT] == FORMAT_FDV3)
          return FDV3_BIND_PERIOD;
      else
          return 26791;
    }
}

static u16 CORONA_build_packet(void) {
    u16 packet_period;
  // Tune frequency if it has been changed
  if (fine != (s8)Model.proto_opts[PROTO_OPTS_FREQFINE]) {
      fine = (s8)Model.proto_opts[PROTO_OPTS_FREQFINE];
      CC2500_WriteReg(CC2500_0C_FSCTRL0, fine);
  }
  // Update RF power
  CC2500_SetPower(Model.tx_power);

  if (state && (Model.proto_opts[PROTO_OPTS_FORMAT] == FORMAT_V2)) {
    // Send identifier packet for 2.65sec. This is how the RX learns the hopping table after a bind. Why it's not part of the bind like V1 is a mistery...
    // Set channel
    CC2500_WriteReg(CC2500_0A_CHANNR, 0x00);
    state--;
    packet[0]=0x07;   // 8 bytes to follow
    // Send hopping freq
    for(u8 i=0; i<CORONA_RF_NUM_CHANNELS; i++)
      packet[i+1]=hopping_frequency[i];
    // Send TX ID
    for(u8 i=0; i<CORONA_ADDRESS_LENGTH; i++)
      packet[i+4]=rx_tx_addr[i];
    packet[8]=0;
    return 6647-CORONA_CHANNEL_TIMING;
  }


  // Flydream every fourth packet is identifier packet and is on channel number
  // TODO that is determined in some unknown manner
  if (Model.proto_opts[PROTO_OPTS_FORMAT] == FORMAT_FDV3 && fdv3_id_send) {
      fdv3_id_send = 0;
      CC2500_WriteReg(CC2500_0A_CHANNR, rx_tx_addr[CORONA_ADDRESS_LENGTH-1]);
//TODO printf("chan: 0x%02x\n", rx_tx_addr[CORONA_ADDRESS_LENGTH-1]);
      packet[0] = 0x07;   // 8 bytes to follow
      // Send TX ID
      for(u8 i = 0; i < CORONA_ADDRESS_LENGTH; i++)
        packet[i+1] = rx_tx_addr[i];
      // Send hopping freq
      for(u8 i = 0; i < CORONA_RF_NUM_CHANNELS; i++)
        packet[i+1+CORONA_ADDRESS_LENGTH] = hopping_frequency[i];
      packet[8] = 0;
      return 2*FDV3_CHANNEL_PERIOD;  // extra delay after id packet according to captures
  }


  // Set RF channel
  CC2500_WriteReg(CC2500_0A_CHANNR, hopping_frequency[hopping_frequency_no]);
//TODO printf("chan: 0x%02x\n", hopping_frequency[hopping_frequency_no]);

  // Build packet
  packet[0] = 0x10;   // 17 bytes to follow
  
  // Channels
  memset(packet+9, 0x00, 4);
  for (u8 i=0; i<8; i++) { // Channel values are packed
    u16 val=convert_channel_ppm(i);
    packet[i+1] = val;
    packet[9 + (i>>1)] |= (i&0x01)?(val>>4)&0xF0:(val>>8)&0x0F;
  }

  // TX ID
  for (u8 i=0; i < CORONA_ADDRESS_LENGTH; i++)
    packet[i+13] = rx_tx_addr[i];
  
  packet[17] = 0x00;

  if (Model.proto_opts[PROTO_OPTS_FORMAT] != FORMAT_FDV3) {
      // Packet period is based on hopping
      switch (hopping_frequency_no) {
        case 0:
          packet_period = Model.proto_opts[PROTO_OPTS_FORMAT] == FORMAT_V1
                        ? 4991-CORONA_CHANNEL_TIMING
                        : 4248-CORONA_CHANNEL_TIMING;
          break;
        case 1: 
          packet_period = Model.proto_opts[PROTO_OPTS_FORMAT] == FORMAT_V1
                        ? 4991-CORONA_CHANNEL_TIMING
                        : 4345-CORONA_CHANNEL_TIMING;
          break;
        case 2: 
          packet_period = Model.proto_opts[PROTO_OPTS_FORMAT] == FORMAT_V1
                        ? 12520-CORONA_CHANNEL_TIMING
                        : 13468-CORONA_CHANNEL_TIMING;
          if (Model.proto_opts[PROTO_OPTS_FORMAT] == FORMAT_V2)
              packet[17] = 0x03;
          break;
      }
  }
  hopping_frequency_no++;
  if (Model.proto_opts[PROTO_OPTS_FORMAT] == FORMAT_FDV3) {
      if (hopping_frequency_no == CORONA_RF_NUM_CHANNELS) {
          fdv3_id_send = 1;
          packet_period = 6000; // extra delay before id packet according to captures
      } else {
          packet_period = FDV3_CHANNEL_PERIOD;
      }
  }
  hopping_frequency_no %= CORONA_RF_NUM_CHANNELS;

  return packet_period;
}

MODULE_CALLTYPE
static u16 corona_cb() {
  u16 packet_period = 0;

  if (bind_counter) {
    bind_counter--;
    if (bind_counter == 0) PROTOCOL_SetBindState(0);
  }
  if (bind_counter)
      packet_period = CORONA_build_bind_pkt();
  else
      packet_period = CORONA_build_packet();

//TODO printf("period: %d, packet: ", packet_period);
//TODO for (u8 i=0; i <= packet[0]+1; i++) printf("%02x ", packet[i]);
//TODO printf("\n");

  // Send packet
  CC2500_WriteData(packet, packet[0]+2);
  packet[0]=0;

#ifndef EMULATOR
  return packet_period;
#else
  return packet_period / 1000;
#endif
}

static void initialize(u8 bind)
{
  CLOCK_StopTimer();

  if (bind) {
      switch (Model.proto_opts[PROTO_OPTS_FORMAT]) {
      case FORMAT_V1:
        PROTOCOL_SetBindState(5000);
        bind_counter = 1400;    // Stay in bind mode for 5s
        break;
      case FORMAT_V2:
        PROTOCOL_SetBindState(5000);
        bind_counter = 187;     // Stay in bind mode for 5s
        break;
      case FORMAT_FDV3:
        PROTOCOL_SetBindState(15000);
        bind_counter = 3000;    // Stay in bind mode for 15s
        break;
      }
  } else {
      bind_counter = 0;
  }
  state = 400;            // Used by V2 to send RF channels + ID for 2.65s at startup
  hopping_frequency_no = 0;
  packet[0] = 0;
  CORONA_init();
  CORONA_rf_init();

  CLOCK_StartTimer(10, corona_cb);
}

const void *Corona_Cmds(enum ProtoCmds cmd)
{
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(0); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(CC2500_Reset() ? 1L : -1L);
        case PROTOCMD_BIND:  initialize(1); return 0;
        case PROTOCMD_NUMCHAN: return (void *)8L;
        case PROTOCMD_DEFAULT_NUMCHAN: return (void *)8L;
        case PROTOCMD_CURRENT_ID: return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_GETOPTIONS: return corona_opts;
        case PROTOCMD_TELEMETRYSTATE: return (void *)(long)PROTO_TELEM_UNSUPPORTED;
        default: break;
    }
    return 0;
}
#endif
