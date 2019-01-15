#include <linux/kernel.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <termios.h>
#include <time.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <sys/types.h>
#include <dirent.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "hciattach.h"
#include <sys/stat.h>

#include "hciattach_sprd.h"

//#include <android/log.h>
//#define DBG
#ifdef DBG
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "pskey_bt", __VA_ARGS__)
#else
#define LOGD(fmt, arg...)  fprintf(stderr, "%s:%d()" fmt "\n", __FILE__,__LINE__, ## arg)
#endif
typedef unsigned char   UINT8;

#define UINT32_TO_STREAM(p, u32) {*(p)++ = (UINT8)(u32); *(p)++ = (UINT8)((u32) >> 8); *(p)++ = (UINT8)((u32) >> 16); *(p)++ = (UINT8)((u32) >> 24);}
#define UINT24_TO_STREAM(p, u24) {*(p)++ = (UINT8)(u24); *(p)++ = (UINT8)((u24) >> 8); *(p)++ = (UINT8)((u24) >> 16);}
#define UINT16_TO_STREAM(p, u16) {*(p)++ = (UINT8)(u16); *(p)++ = (UINT8)((u16) >> 8);}
#define UINT8_TO_STREAM(p, u8)   {*(p)++ = (UINT8)(u8);}
#define INT8_TO_STREAM(p, u8)    {*(p)++ = (INT8)(u8);}

#define PSKEY_PRELOAD_SIZE    0x04
#define PSKEY_PREAMBLE_SIZE    0xA2

	// for bt mac addr
#define BT_MAC_FILE_PATH	"/csa/bluetooth/"
#define DATMISC_MAC_ADDR_PATH	BT_MAC_FILE_PATH".bd_addr"
#define MAC_ADDR_BUF_LEN    (strlen("FF:FF:FF:FF:FF:FF"))
#define MAC_ADDR_FILE_LEN    25
#define MAC_ADDR_LEN    6

#define BD_ADDR_LEN	14
#define BD_PREFIX	"0002\n"

#if 0
#ifndef VENDOR_BTWRITE_PROC_NODE
#define VENDOR_BTWRITE_PROC_NODE "/proc/bluetooth/sleep/btwrite"
#endif
#endif

#define MAX_BT_TMP_PSKEY_FILE_LEN 2048

typedef unsigned int   UWORD32;
typedef unsigned short UWORD16;
typedef unsigned char  UWORD8;

#define down_bt_is_space(c)	(((c) == '\n') || ((c) == ',') || ((c) == '\r') || ((c) == ' ') || ((c) == '{') || ((c) == '}'))
#define down_bt_is_comma(c)	(((c) == ','))
#define down_bt_is_endc(c)	(((c) == '}')) // indicate end of data

/* Macros to swap byte order */
#define SWAP_BYTE_ORDER_WORD(val) ((((val) & 0x000000FF) << 24) + \
                                   (((val) & 0x0000FF00) << 8)  + \
                                   (((val) & 0x00FF0000) >> 8)   + \
                                   (((val) & 0xFF000000) >> 24))
#define INLINE static __inline

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN
#endif


// pskey file structure default value
static BT_PSKEY_CONFIG_T bt_para_setting={
	.pskey_cmd = 0x001C0101,

	.g_dbg_source_sink_syn_test_data = 0,
	.g_sys_sleep_in_standby_supported = 0,
	.g_sys_sleep_master_supported = 0,
	.g_sys_sleep_slave_supported = 0,

	.default_ahb_clk = 26000000,
	.device_class = 0x001F00,
	.win_ext = 30,

	.g_aGainValue = {0x0000F600, 0x0000D000, 0x0000AA00, 0x00008400, 0x00004400, 0x00000A00},
	.g_aPowerValue = {0x0FC80000, 0x0FF80000, 0x0FDA0000, 0x0FCC0000, 0x0FFC0000},

	.feature_set = {0xFF, 0xFF, 0x8D, 0xFE, 0x9B, 0x7F, 0x79, 0x83, 0xFF, 0xA7, 0xFF, 0x7F, 0x00, 0xE0, 0xF7, 0x3E},
	.device_addr = {0x6A, 0x6B, 0x8C, 0x8A, 0x8B, 0x8C},

	.g_sys_sco_transmit_mode = 0, //true tramsmit by uart, otherwise by share memory
	.g_sys_uart0_communication_supported = 1, //true use uart0, otherwise use uart1 for debug
	.edr_tx_edr_delay = 5,
	.edr_rx_edr_delay = 14,

	.g_wbs_nv_117 = 0x0031,

	.is_wdg_supported = 0,

	.share_memo_rx_base_addr = 0,
	//.share_memo_tx_base_addr = 0,
	.g_wbs_nv_118 = 0x0066,
	.g_nbv_nv_117 = 0x1063,

	.share_memo_tx_packet_num_addr = 0,
	.share_memo_tx_data_base_addr = 0,

	.g_PrintLevel = 0xFFFFFFFF,

	.share_memo_tx_block_length = 0,
	.share_memo_rx_block_length = 0,
	.share_memo_tx_water_mark = 0,
	//.share_memo_tx_timeout_value = 0,
	.g_nbv_nv_118 = 0x0E45,

	.uart_rx_watermark = 48,
	.uart_flow_control_thld = 63,
	.comp_id = 0,
	.pcm_clk_divd = 0x26,


	.reserved = {0}
};

extern int getPskeyFromFile(void *pData);
extern int bt_getPskeyFromFile(void *pData);

static int create_mac_folder(void)
{
	DIR *dp;
	int err;

	dp = opendir(BT_MAC_FILE_PATH);
	if (dp == NULL) {
		if (mkdir(BT_MAC_FILE_PATH, 0755) < 0) {
			err = -errno;
			LOGD("%s:  mkdir: %s(%d)",__FUNCTION__, strerror(-err), -err);
		}
		return -1;
	}

	closedir(dp);
	return 0;
}

static void mac_rand(char *btmac)
{
	int ran;
	int i;
	unsigned int seed;
	struct timeval tv;

	memcpy(btmac, BD_PREFIX, 5);
	i = gettimeofday(&tv, NULL);

	if (i < 0) {
		LOGD("Fail to call gettimeofday()");
		seed = time(NULL);
	} else
		seed = (unsigned int)tv.tv_usec;

	for (i = 5; i < BD_ADDR_LEN; i++) {
		if (i == 7) {
			btmac[i] = '\n';
			continue;
		}
		ran = rand_r(&seed) % 16;
		if (ran < 10)
			ran += 0x30;
		else
			ran += 0x57;
		btmac[i] = ran;
	}
	LOGD("Random number is\r\n");
	for (i = 0; i < BD_ADDR_LEN; i++) {
		LOGD("%c", btmac[i]);
	}
	LOGD("\r\n");
}

static void write_btmac2file(char *btmac)
{
	int fd;
	int ret;
	fd = open(DATMISC_MAC_ADDR_PATH, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR);
	LOGD("write_btmac2file open file, fd=%d", fd);
	if(fd >= 0) {
		if(chmod(DATMISC_MAC_ADDR_PATH,0666) != -1){
			ret = write(fd, btmac, strlen(btmac));
			if (ret < strlen(btmac)) {
				LOGD("Fail to write %s", DATMISC_MAC_ADDR_PATH);
				close(fd);
				return;
			}
		}
		close(fd);
	}else{
		LOGD("write bt mac to file failed!!");
	}
}

uint8 ConvertHexToBin(
		uint8        *hex_ptr,     // in: the hexadecimal format string
		uint16       length,       // in: the length of hexadecimal string
		uint8        *bin_ptr      // out: pointer to the binary format string
		){
	uint8        *dest_ptr = bin_ptr;
	uint32        i = 0;
	uint8        ch;

	for(i=0; i<length; i+=2){
		// the bit 8,7,6,5
		ch = hex_ptr[i];
		// digital 0 - 9
		if (ch >= '0' && ch <= '9')
			*dest_ptr =(uint8)((ch - '0') << 4);
		// a - f
		else if (ch >= 'a' && ch <= 'f')
			*dest_ptr = (uint8)((ch - 'a' + 10) << 4);
		// A - F
		else if (ch >= 'A' && ch <= 'F')
			*dest_ptr = (uint8)((ch -'A' + 10) << 4);
		else{
			return 0;
		}

		// the bit 1,2,3,4
		ch = hex_ptr[i+1];
		// digtial 0 - 9
		if (ch >= '0' && ch <= '9')
			*dest_ptr |= (uint8)(ch - '0');
		// a - f
		else if (ch >= 'a' && ch <= 'f')
			*dest_ptr |= (uint8)(ch - 'a' + 10);
		// A - F
		else if (ch >= 'A' && ch <= 'F')
			*dest_ptr |= (uint8)(ch -'A' + 10);
		else{
			return 0;
		}

		dest_ptr++;
	}

	return 1;
}

static int read_mac_address(char *file_name, uint8 *addr) {
	char buf[MAC_ADDR_FILE_LEN] = {0};
	uint32 addr_t[MAC_ADDR_LEN] = {0};
	int i = 0;


#if 1
	int fd = open(file_name, O_RDONLY, 0666);
	LOGD("%s read file: %s", __func__, file_name);
	if (fd < 0) {
		LOGD("%s open %s error reason: %s", __func__, file_name, strerror(errno));
		return -1;
	}
	if (read(fd, buf, BD_ADDR_LEN) < 0) {
		LOGD("%s read %s error reason: %s", __func__, file_name, strerror(errno));
		goto done;
	}
	if (sscanf(buf, "%02X%02X\n%02X\n%02X%02X%02X", &addr_t[0], &addr_t[1], &addr_t[2], &addr_t[3], &addr_t[4], &addr_t[5]) < 0) {
		LOGD("%s sscanf %s error reason: %s", __func__, file_name, strerror(errno));
		goto done;
	}

	for (i = 0; i < MAC_ADDR_LEN; i++) {
		addr[i] = addr_t[i] & 0xFF;
	}
	LOGD("%s %s addr: [%02X:%02X:%02X:%02X:%02X:%02X]", __func__, file_name, addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

done:
	close(fd);
#endif
	return 0;
}

static void mac_address_stream_compose(uint8 *addr) {
	uint8 tmp, i, j;
	for (i = 0, j = MAC_ADDR_LEN - 1; (i < MAC_ADDR_LEN / 2) && (i != j); i++, j--) {
		tmp = addr[i];
		addr[i] = addr[j];
		addr[j] = tmp;
	}
}

#if 0
/*
 * random bluetooth mac address
 */
static void random_mac_addr(uint8 *addr) {
	int fd, randseed, ret, mac_rd;
	uint8 addr_t[MAC_ADDR_LEN] = {0};

	LOGD("%s", __func__);
	/* urandom seed build */
	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0){
		LOGD("%s: open urandom fail", __func__);
	} else {
		ret = read(fd, &randseed, sizeof(randseed));
		LOGD("%s urandom:0x%08X", __func__, randseed);
		close(fd);
	}

	/* time seed build */
	if (fd < 0 || ret < 0) {
		struct timeval tt;
		if (gettimeofday(&tt, (struct timezone *)0) > 0) {
			randseed = (unsigned int) tt.tv_usec;
		} else {
			randseed = (unsigned int) time(NULL);
		}
		LOGD("urandom fail, using system time for randseed");
	}

	LOGD("%s: randseed = %u",__func__, randseed);
	srand(randseed);
	mac_rd = rand();

	addr_t[0] = 0x40; /* FOR */
	addr_t[1] = 0x45; /* SPRD */
	addr_t[2] = 0xDA; /* ADDR */
	addr_t[3] = (uint8)(mac_rd & 0xFF);
	addr_t[4] = (uint8)((mac_rd >> 8) & 0xFF);
	addr_t[5] = (uint8)((mac_rd >> 16) & 0xFF);

	memcpy(addr, addr_t, MAC_ADDR_LEN);
	LOGD("%s: MAC ADDR: [%02X:%02X:%02X:%02X:%02X:%02X]",__func__, addr_t[0], addr_t[1], addr_t[2], addr_t[3], addr_t[4], addr_t[5]);
}
#endif
static void get_mac_address(uint8 *addr){
	int ret = -1;
	uint8 addr_t[6] = {0};
	char bt_mac[BD_ADDR_LEN] = {0, };

	LOGD("%s", __func__);
	/* check misc mac file exist */
	ret = access(DATMISC_MAC_ADDR_PATH, F_OK);
	if (ret != 0) {
		LOGD("%s %s miss", __func__, DATMISC_MAC_ADDR_PATH);

		/* Try to make bt address file */
		create_mac_folder();

		mac_rand(bt_mac);
		LOGD("bt random mac=%s",bt_mac);
		write_btmac2file(bt_mac);

	}

	/* read mac file */
	read_mac_address(DATMISC_MAC_ADDR_PATH, addr_t);

	/* compose mac stream */
	mac_address_stream_compose(addr_t);

	memcpy(addr, addr_t, MAC_ADDR_LEN);

}


/*
 * hci command preload stream,  special order
 */
static void pskey_stream_compose(uint8 * buf, BT_PSKEY_CONFIG_T *bt_par) {
	int i = 0;
	uint8 *p = buf;

	LOGD("%s", __func__);

	UINT24_TO_STREAM(p, bt_par->pskey_cmd);
	UINT8_TO_STREAM(p, (uint8)(PSKEY_PREAMBLE_SIZE & 0xFF));

	UINT8_TO_STREAM(p, bt_par->g_dbg_source_sink_syn_test_data);
	UINT8_TO_STREAM(p, bt_par->g_sys_sleep_in_standby_supported);
	UINT8_TO_STREAM(p, bt_par->g_sys_sleep_master_supported);
	UINT8_TO_STREAM(p, bt_par->g_sys_sleep_slave_supported);

	UINT32_TO_STREAM(p, bt_par->default_ahb_clk);
	UINT32_TO_STREAM(p, bt_par->device_class);
	UINT32_TO_STREAM(p, bt_par->win_ext);

	for (i = 0; i < 6; i++) {
		UINT32_TO_STREAM(p, bt_par->g_aGainValue[i]);
	}
	for (i = 0; i < 5; i++) {
		UINT32_TO_STREAM(p, bt_par->g_aPowerValue[i]);
	}

	for (i = 0; i < 16; i++) {
		UINT8_TO_STREAM(p, bt_par->feature_set[i]);
	}
	for (i = 0; i < 6; i++) {
		UINT8_TO_STREAM(p, bt_par->device_addr[i]);
	}

	UINT8_TO_STREAM(p, bt_par->g_sys_sco_transmit_mode);
	UINT8_TO_STREAM(p, bt_par->g_sys_uart0_communication_supported);
	UINT8_TO_STREAM(p, bt_par->edr_tx_edr_delay);
	UINT8_TO_STREAM(p, bt_par->edr_rx_edr_delay);

	UINT16_TO_STREAM(p, bt_par->g_wbs_nv_117);

	UINT32_TO_STREAM(p, bt_par->is_wdg_supported);

	UINT32_TO_STREAM(p, bt_par->share_memo_rx_base_addr);
	//UINT32_TO_STREAM(p, bt_par->share_memo_tx_base_addr);
	UINT16_TO_STREAM(p, bt_par->g_wbs_nv_118);
	UINT16_TO_STREAM(p, bt_par->g_nbv_nv_117);

	UINT32_TO_STREAM(p, bt_par->share_memo_tx_packet_num_addr);
	UINT32_TO_STREAM(p, bt_par->share_memo_tx_data_base_addr);

	UINT32_TO_STREAM(p, bt_par->g_PrintLevel);

	UINT16_TO_STREAM(p, bt_par->share_memo_tx_block_length);
	UINT16_TO_STREAM(p, bt_par->share_memo_rx_block_length);
	UINT16_TO_STREAM(p, bt_par->share_memo_tx_water_mark);
	//UINT16_TO_STREAM(p, bt_par->share_memo_tx_timeout_value);
	UINT16_TO_STREAM(p, bt_par->g_nbv_nv_118);

	UINT16_TO_STREAM(p, bt_par->uart_rx_watermark);
	UINT16_TO_STREAM(p, bt_par->uart_flow_control_thld);
	UINT32_TO_STREAM(p, bt_par->comp_id);
	UINT16_TO_STREAM(p, bt_par->pcm_clk_divd);


	for (i = 0; i < 8; i++) {
		UINT32_TO_STREAM(p, bt_par->reserved[i]);
	}
}

void sprd_get_pskey(BT_PSKEY_CONFIG_T * pskey_t) {
	BT_PSKEY_CONFIG_T pskey;
	uint8 buf[180] = {0};

	LOGD("%s", __func__);
	memset(&pskey, 0 , sizeof(BT_PSKEY_CONFIG_T));
	if (bt_getPskeyFromFile(&pskey) < 0 ) {
		LOGD("%s bt_getPskeyFromFile failed", __func__);
		memcpy(pskey_t, &bt_para_setting, sizeof(BT_PSKEY_CONFIG_T));
		return;
	}

	memset(buf, 0, PSKEY_PRELOAD_SIZE + PSKEY_PREAMBLE_SIZE);

	/* get bluetooth mac address */
	get_mac_address(pskey.device_addr);

	/* compose pskey hci command pkt */
	pskey_stream_compose(buf, &pskey);

	memcpy(pskey_t, &pskey, sizeof(BT_PSKEY_CONFIG_T));
}

#define HCI_HDR_LEN	3

int sprd_config_init(int fd, char *bdaddr, struct termios *ti)
{
	int ret = 0,r=0;
	unsigned char resp[30] = {0};
	BT_PSKEY_CONFIG_T bt_para_tmp;
	uint8 data_tmp[30] = {'a'};
	static int index = 0;
	uint8 *buf = NULL;
	uint8 hci_len = 0;
	uint8 is_expected_hci_evt = 0;
#if 0
	char buffer;
	int btsleep_fd_sprd = -1;
#endif
	LOGD("sprd_config_init");

#if 0
	uart_fd = open(UART_INFO_PATH, O_WRONLY);
	if(uart_fd > 0)
	{
		buffer = '2';
		if (write(uart_fd, &buffer, 1) < 0)
		{
			LOGD("%s write(%s) failed: %s (%d) 2", __func__,
					UART_INFO_PATH, strerror(errno),errno);
		}

		close(uart_fd);
	}
#endif

#if 0
	btsleep_fd_sprd = open(VENDOR_BTWRITE_PROC_NODE, O_WRONLY);
	if (btsleep_fd_sprd < 0)
	{
		LOGD("%s open(%s) for write failed: %s (%d)", __func__,
				VENDOR_BTWRITE_PROC_NODE, strerror(errno), errno);
	}
	else
	{
		buffer = '1';
		if (write(btsleep_fd_sprd, &buffer, 1) < 0)
		{
			LOGD("%s write(%s) failed: %s (%d)", __func__,
					VENDOR_BTWRITE_PROC_NODE, strerror(errno),errno);
		}
	}
#endif

	ret = bt_getPskeyFromFile(&bt_para_tmp);
	if (ret < 0) {
		LOGD("init_sprd_config bt_getPskeyFromFile failed\n");
		memcpy(&bt_para_tmp, &bt_para_setting, sizeof(BT_PSKEY_CONFIG_T));
	}

	buf = (uint8 *)malloc(PSKEY_PRELOAD_SIZE + PSKEY_PREAMBLE_SIZE);
	if (buf == NULL) {
		LOGD("%s alloc stream memory failed", __func__);
		return -1;
	}
	memset(buf, 0, PSKEY_PRELOAD_SIZE + PSKEY_PREAMBLE_SIZE);

	/* get bluetooth mac address */
	get_mac_address(bt_para_tmp.device_addr);

	/* compose pskey hci command pkt */
	pskey_stream_compose(buf, &bt_para_tmp);

	ret = write(fd, buf, PSKEY_PRELOAD_SIZE + PSKEY_PREAMBLE_SIZE);
	LOGD("write pskey ret = %d", ret);

	free(buf);
	buf = NULL;

	if (ret < 0) {
		LOGD("%s write pskey stream failed", __func__);
		return -1;
	}

	memset(data_tmp, 0xff, sizeof(data_tmp));
	while (1) {
		r = read(fd, resp, 1);

		if (r <= 0)
			return -1;
		else{
			data_tmp[index] = resp[0];
			LOGD("recive from controller 0x%x", data_tmp[index]);
			++index;
		}

		if (index >= 6) {
			hci_len = data_tmp[2]+HCI_HDR_LEN;

			if ((data_tmp[0] == 0x04) && (data_tmp[1] == 0xe) &&
				(data_tmp[2] == 0xa) &&(data_tmp[3] == 0x1) &&
				(data_tmp[4] == 0xa0) &&(data_tmp[5] == 0xfc)) {
					LOGD("read response ok \n");
					is_expected_hci_evt = 1;
				} else {
					LOGD("this is not what we expect HCI evt\n");
					is_expected_hci_evt = 0;
				}

			if (index == hci_len) {
				index = 0;
				memset(data_tmp, 0x0, sizeof(data_tmp));

				if(is_expected_hci_evt)
					break;
			}
		}
	}
	return 0;
}
