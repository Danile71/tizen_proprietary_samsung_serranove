#ifndef HCIATTACH_SPRD_H__
#define HCIATTACH_SPRD_H__

#define MAC_ERROR		"FF:FF:FF:FF:FF:FF"

#define BT_MAC_FILE		"/productinfo/btmac.txt"
//#define GET_BTMAC_ATCMD	"AT+SNVM=0,401"
//#define GET_BTPSKEY_ATCMD	"AT+SNVM=0,415"
//#define SET_BTMAC_ATCMD	"AT+SNVM=1,401"
#define BT_RAND_MAC_LENGTH   17

// used to store BT pskey structure and default values
#define BT_PSKEY_STRUCT_FILE "/system/lib/modules/pskey_bt.txt"
//#define BT_PSKEY_FILE	"/system/lib/modules/pskey_bt.txt"


typedef unsigned char uint8;
typedef unsigned int uint32;
typedef unsigned short uint16;

#define BT_ADDRESS_SIZE    6


typedef struct SPRD_BT_PSKEY_INFO_T{
	uint32	 pskey_cmd;

	uint8    g_dbg_source_sink_syn_test_data;
	uint8    g_sys_sleep_in_standby_supported;
	uint8    g_sys_sleep_master_supported;
	uint8    g_sys_sleep_slave_supported;

	uint32  default_ahb_clk;
	uint32  device_class;
	uint32  win_ext;

	uint32  g_aGainValue[6];
	uint32  g_aPowerValue[5];

	uint8    feature_set[16];
	uint8    device_addr[6];

	uint8    g_sys_sco_transmit_mode; //true tramsmit by uart, otherwise by share memory
	uint8    g_sys_uart0_communication_supported; //true use uart0, otherwise use uart1 for debug
	uint8    edr_tx_edr_delay;
	uint8    edr_rx_edr_delay;

	uint16  g_wbs_nv_117;

	uint32  is_wdg_supported;

	uint32  share_memo_rx_base_addr;

	// uint32  share_memo_tx_base_addr;
	uint16  g_wbs_nv_118;
	uint16  g_nbv_nv_117;

	uint32  share_memo_tx_packet_num_addr;
	uint32  share_memo_tx_data_base_addr;

	uint32  g_PrintLevel;

	uint16  share_memo_tx_block_length;
	uint16  share_memo_rx_block_length;
	uint16  share_memo_tx_water_mark;

	//uint16  share_memo_tx_timeout_value;
	uint16  g_nbv_nv_118;

	uint16  uart_rx_watermark;
	uint16  uart_flow_control_thld;
	uint32  comp_id;
	uint16  pcm_clk_divd;

	uint32  reserved[8];
}BT_PSKEY_CONFIG_T;


#endif /* HCIATTACH_SPRD_H__ */




