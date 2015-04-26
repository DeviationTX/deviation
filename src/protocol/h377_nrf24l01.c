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
  #define H377_Cmds PROTO_Cmds
  #pragma long_calls
#endif
#include "common.h"
#include "interface.h"
#include "mixer.h"
#include "config/model.h"
#include "config/tx.h" // for Transmitter
#include "music.h"

#ifdef MODULAR
  //Some versions of gcc apply this to definitions, others to calls
  //So just use long_calls everywhere
  //#pragma long_calls_off
  extern unsigned _data_loadaddr;
  const unsigned long protocol_type = (unsigned long)&_data_loadaddr;
#endif

#ifdef PROTO_HAS_NRF24L01

#include "iface_nrf24l01.h"

#ifdef EMULATOR
#define USE_FIXED_MFGID
#define BIND_COUNT 5
#else
#define BIND_COUNT 800
#endif

static int counter;

// Bit vector from bit position
#define BV(bit) (1 << bit)

#define TXID_SIZE 5

#define FREQUENCE_NUM  20
#define SET_NUM  9
// available frequency must be in between 2402 and 2477

static u8 binding_ch=0x50;
static u8 hopping_frequency[FREQUENCE_NUM];
static u8 hopping_frequency_no;
static u8 hopping_frequency_data[SET_NUM] = {0x1c,0x1b,0x1d,0x11,0x0e,0x0d,0x01,0x1d,0x15};

static const u8  binding_adr_rf[5]={0x32,0xaa,0x45,0x45,0x78};

static u8 rf_adr_buf[5]; 
static u8 rf_adr_buf_data[SET_NUM][5] = {
	{0xad,0x9a,0xa6,0x69,0xb2},//ansheng
	{0x92,0x9a,0x9d,0x69,0x99},//dc59
	{0x92,0xb2,0x9d,0x69,0x9a},//small two
	{0xad,0x9a,0x5a,0x69,0x96},//james_1
	{0x95,0x9a,0x52,0x69,0x99},//james_2
	{0x52,0x52,0x52,0x69,0xb9},//james_3
	{0x52,0x52,0x52,0x52,0x55},//small two_1
	{0x92,0xB2,0x9D,0x69,0x9A},//small two_2
	{0x96,0x9A,0x45,0x69,0xB2}//small two_3
	};	

static u8 bind_buf_arry[10];
static u8 bind_buf_arry_data[SET_NUM][4] = {
	{0xcf,0x1c,0x19,0x1a},
	{0xff,0x48,0x19,0x19},
	{0xf3,0x4d,0x19,0x19},
	{0x9e,0x1f,0x19,0x19},
	{0x8d,0x3d,0x19,0x19},
	{0xbd,0x23,0x19,0x19},
	{0xF3,0x28,0x19,0x19},
	{0xF3,0x4D,0x19,0x19},
	{0x82,0x8D,0x19,0x19}};
   

static unsigned int ch_data[8];
static u8 payload[10];
static u8 counter1ms;

static int select_ch_id = 0;

static void calc_fh_channels()
{       

    printf("=>H377 : calc_fh_channels\n");
	hopping_frequency[0] = hopping_frequency_data[select_ch_id];
    
    for (int i = 1; i < FREQUENCE_NUM; i++) 
    {
        hopping_frequency[i] = hopping_frequency[i-1] + 3;        
    } 

	 printf("=>H377 : FH Seq(hopping_frequency): ");
    for (int i = 0; i < FREQUENCE_NUM; ++i) {
        printf("%d(0x%02x), ", hopping_frequency[i], hopping_frequency[i]);
    }
    printf(" \r\n");
}


static void build_binding_packet(void) //bind_buf_arry
{
    u8 i;
    //printf("=>H377 : build_binding_packet\n");
    counter1ms = 0;
    hopping_frequency_no = 0;

    for(i=0;i<5;i++)
      bind_buf_arry[i] = rf_adr_buf[i];
  
    bind_buf_arry[5] = hopping_frequency[0];
	
	for(i=0;i<4;i++)
      bind_buf_arry[i+6] = bind_buf_arry_data[select_ch_id][i];
	

    printf("==>bind_buf_arry\n");
    for(int n=0; n<10; n++)
    {
         printf(" 0x%02x(%d), ", bind_buf_arry[n], bind_buf_arry[n]);
    }
    printf("\n");

    printf("<==bind_buf_arry\n");

}

static void h377_init()
{
    printf("=>H377 : h377_init\n");
    NRF24L01_Initialize();

    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable p0 rx
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rf_adr_buf, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rf_adr_buf, 5);
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, 10); // payload size = 10
    //NRF24L01_WriteReg(NRF24L01_05_RF_CH, 81); // binding packet must be set in channel 81
	NRF24L01_WriteReg(NRF24L01_05_RF_CH, binding_ch); // binding packet must be set in channel 81

    // 2-bytes CRC, radio off
    NRF24L01_SetTxRxMode(TX_EN);
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PWR_UP));
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address (byte -2)
    NRF24L01_SetBitrate(0);                          // 1Mbps
    NRF24L01_SetPower(Model.tx_power);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit


    // Check for Beken BK2421/BK2423 chip
    // It is done by using Beken specific activate code, 0x53
    // and checking that status register changed appropriately
    // There is no harm to run it on nRF24L01 because following
    // closing activate command changes state back even if it
    // does something on nRF24L01
    // For detailed description of what's happening here see 
    //   http://www.inhaos.com/uploadfile/otherpic/AN0008-BK2423%20Communication%20In%20250Kbps%20Air%20Rate.pdf
    NRF24L01_Activate(0x53); // magic for BK2421 bank switch
    printf("=>H377 : Trying to switch banks\n");
    if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & 0x80) {
        printf("=>H377 : BK2421 detected\n");
        long nul = 0;
        // Beken registers don't have such nice names, so we just mention
        // them by their numbers
        // It's all magic, eavesdropped from real transfer and not even from the
        // data sheet - it has slightly different values
        NRF24L01_WriteRegisterMulti(0x00, (u8 *) "\x40\x4B\x01\xE2", 4);
        NRF24L01_WriteRegisterMulti(0x01, (u8 *) "\xC0\x4B\x00\x00", 4);
        NRF24L01_WriteRegisterMulti(0x02, (u8 *) "\xD0\xFC\x8C\x02", 4);
        NRF24L01_WriteRegisterMulti(0x03, (u8 *) "\xF9\x00\x39\x21", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xC1\x96\x9A\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x05, (u8 *) "\x24\x06\x7F\xA6", 4);
        NRF24L01_WriteRegisterMulti(0x06, (u8 *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x07, (u8 *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x08, (u8 *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x09, (u8 *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x0A, (u8 *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x0B, (u8 *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x0C, (u8 *) "\x00\x12\x73\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0D, (u8 *) "\x46\xB4\x80\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0E, (u8 *) "\x41\x10\x04\x82\x20\x08\x08\xF2\x7D\xEF\xFF", 11);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xC7\x96\x9A\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x04, (u8 *) "\xC1\x96\x9A\x1B", 4);
    } else {
        printf("=>H377 : nRF24L01 detected\n");
    }
    NRF24L01_Activate(0x53); // switch bank back
}

// H377 channel sequence: AILE  ELEV  THRO  RUDD  GEAR  PITH, channel data value is from 0 to 1000
static void build_ch_data()
{
    s32 temp;
    u8 i;
    //printf("=>H377 : build_ch_data\n");
    for (i = 0; i< 8; i++) {
        if (i >= Model.num_channels)
            ch_data[i] = 500; // any data between 0 to 1000 is ok
        else 
        {
            temp = (s32)Channels[i] * 450/CHAN_MAX_VALUE + 500; // max/min servo range is +-125%
            if (i == 2) // It is clear that h377's thro stick is made reversely, so I adjust it here on purpose
                temp = 1000 -temp;
            //if (i == 0) // It is clear that h377's thro stick is made reversely, so I adjust it here on purpose
            //    temp = 1000 -temp;
            //if (i == 1) // It is clear that h377's thro stick is made reversely, so I adjust it here on purpose
            //    temp = 1000 -temp;
            if (temp < 0)
                ch_data[i] = 0;
            else if (temp > 1000)
                ch_data[i] = 1000;
            else
                ch_data[i] = (unsigned int)temp;
        }

        payload[i] = (u8)ch_data[i];
    }

    payload[8]  = (u8)((ch_data[0]>>8)&0x0003);
    payload[8] |= (u8)((ch_data[1]>>6)&0x000c);
    payload[8] |= (u8)((ch_data[2]>>4)&0x0030);
    payload[8] |= (u8)((ch_data[3]>>2)&0x00c0);

    payload[9]  = (u8)((ch_data[4]>>8)&0x0003);
    payload[9] |= (u8)((ch_data[5]>>6)&0x000c);
    payload[9] |= (u8)((ch_data[6]>>4)&0x0030);
    payload[9] |= (u8)((ch_data[7]>>2)&0x00c0);

#ifdef EMULATOR
    for (i = 0; i < 8; i++)
        printf("=>H377 : ch[%d]=%d,  payload[%d]=%d\n", i, ch_data[i], i, payload[i]);
    printf("=>H377 : payload[8]=%d\n", payload[8]);
    printf("=>H377 : payload[9]=%d\n", payload[9]);
#endif

}

MODULE_CALLTYPE
static u16 h377_cb()
{    
    counter1ms++;
    //printf("##[%d, %d]>H377 : h377_cb \n", counter1ms, counter);
    if(counter1ms==1)
    {
        NRF24L01_FlushTx();
    }
    //-------------------------
    else if(counter1ms==2) 
    {
        if (counter>0) 
        {
            NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, (u8 *)binding_adr_rf, 5);
            NRF24L01_WriteReg(NRF24L01_05_RF_CH, binding_ch);
        }
    }
    else if(counter1ms==3) 
    {
        if (counter >0)
        {
            counter--;
            if (! counter) 
            { // binding finished, change tx add
                PROTOCOL_SetBindState(0);
            }
            NRF24L01_WritePayload(bind_buf_arry,10);
        }
 
    } 
    else if (counter1ms==4) 
    {
        if (counter > 0)
        {
            NRF24L01_FlushTx();
        }
    }
    //-------------------------
    else if(counter1ms==5)
    {
        NRF24L01_SetPower(Model.tx_power);
    }
    //-------------------------
    else if (counter1ms == 6) 
    {
        NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rf_adr_buf, 5);
        NRF24L01_WriteReg(NRF24L01_05_RF_CH, hopping_frequency[hopping_frequency_no]);
        hopping_frequency_no++;
        if (hopping_frequency_no >= FREQUENCE_NUM)
        {
            hopping_frequency_no = 0;
        }
    }
    else if (counter1ms == 7) 
    {
        build_ch_data();
    }
    else if(counter1ms>8)
    {
        counter1ms = 0;
        NRF24L01_WritePayload(payload,10);
    }
#ifdef EMULATOR
    return 100;
#else
    return 1000;  // send 1 binding packet and 1 data packet per 9ms
#endif
}

// Linear feedback shift register with 32-bit Xilinx polinomial x^32 + x^22 + x^2 + x + 1
static const uint32_t LFSR_FEEDBACK = 0x80200003ul;
static const uint32_t LFSR_INTAP = 32-1;

static void update_lfsr(uint32_t *lfsr, uint8_t b)
{
    for (int i = 0; i < 8; ++i) {
        *lfsr = (*lfsr >> 1) ^ ((-(*lfsr & 1u) & LFSR_FEEDBACK) ^ ~((uint32_t)(b & 1) << LFSR_INTAP));
        b >>= 1;
    }
}

// Generate internal id from TX id and manufacturer id (STM32 unique id)


static void initialize_tx_id()
{
	u8 i;
	for(i=0;i<5;i++)
      rf_adr_buf[i] = rf_adr_buf_data[select_ch_id][i];    
      
    printf("=2=>H377 : Effective id(rf_adr_buf): 0x%02X(%d), 0x%02X(%d), 0x%02X(%d), 0x%02X(%d), 0x%02X(%d)\r\n",
        rf_adr_buf[0], rf_adr_buf[0], rf_adr_buf[1], rf_adr_buf[1], rf_adr_buf[2], rf_adr_buf[2], rf_adr_buf[3], rf_adr_buf[3], rf_adr_buf[4], rf_adr_buf[4]);

    calc_fh_channels();  
}



static void initialize(u8 bind)
{
    printf("=>H377 : initialize, bind=0x%02x, Model.fixed_id=0x%02x \n", bind, Model.fixed_id);
	u32 lfsr = 0x7649eca9ul;
    select_ch_id = Model.fixed_id;

	if (Model.fixed_id == 0) 
    {
		for (u8 i = 0; i < TXID_SIZE; ++i) 		
			update_lfsr(&lfsr, i);   
		select_ch_id = lfsr%SET_NUM;
    }
	else
	{
		select_ch_id = Model.fixed_id%SET_NUM;
	}
	printf("=>H377 : select_ch_id = %d \n", select_ch_id);
    CLOCK_StopTimer();

    initialize_tx_id();//rf_adr_buf  hopping_frequency

    build_binding_packet();//bind_buf_arry (rf_adr_buf hopping_frequency)
      
    h377_init();

    if(bind || ! Model.fixed_id || ((Model.fixed_id<(100 + SET_NUM))&&(Model.fixed_id>=100))) 
    {
        printf("=>H377 : initialize((bind || ! Model.fixed_id) \n");
        counter = BIND_COUNT;
        PROTOCOL_SetBindState((BIND_COUNT > 200 ? BIND_COUNT : 800) * 10); //8 seconds binding time
    } 
    else 
    {
        counter = 0;
    }
    printf("=>H377 : counter = %d\n", counter);

    CLOCK_StartTimer(1000, h377_cb);
}

const void *H377_Cmds(enum ProtoCmds cmd)
{
     
    switch(cmd) {
        case PROTOCMD_INIT:  initialize(0); printf("=>H377 : cmd %d PROTOCMD_INIT\n", cmd); return 0;
        case PROTOCMD_DEINIT:
        case PROTOCMD_RESET:
            CLOCK_StopTimer();
            return (void *)(NRF24L01_Reset() ? 1L : -1L);
        case PROTOCMD_CHECK_AUTOBIND: printf("=>H377 : cmd %d PROTOCMD_CHECK_AUTOBIND\n", cmd); return (void *)0L; //Never Autobind
        case PROTOCMD_BIND:  initialize(1); printf("=>H377 : cmd %d PROTOCMD_BIND\n", cmd); return 0;
        case PROTOCMD_NUMCHAN: printf("=>H377 : cmd %d PROTOCMD_NUMCHAN\n", cmd); return (void *)6L;
        case PROTOCMD_DEFAULT_NUMCHAN: printf("=>H377 : cmd %d PROTOCMD_DEFAULT_NUMCHAN\n", cmd); return (void *)6L;
        case PROTOCMD_CURRENT_ID: printf("=>H377 : cmd %d PROTOCMD_CURRENT_ID\n", cmd); return Model.fixed_id ? (void *)((unsigned long)Model.fixed_id) : 0;
        case PROTOCMD_TELEMETRYSTATE: printf("=>H377 : cmd %d PROTOCMD_TELEMETRYSTATE\n", cmd); return (void *)(long)-1;
        default: break;
    }
    return 0;
}
#endif
