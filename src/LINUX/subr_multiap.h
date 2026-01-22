#ifndef _SUBR_MULTIAP_H_
#define _SUBR_MULTIAP_H_

#include <stdint.h>

//#if defined(CONFIG_USER_CTCAPD) && defined(RTK_MULTI_AP)
//#define WLAN_MULTI_AP_ROLE_AUTO_SELECT
//#endif

#define MAP_CONFIG_2G                 (0)
#define MAP_CONFIG_5GL                (1)
#define MAP_CONFIG_5GH                (2)
#define MAP_CONFIG_5GF                (3)

#if defined(WLAN_MULTI_AP_ROLE_AUTO_SELECT)
#define ROLE_QUERY_TIME				  10
#define ROLE_QUERY_INTERVAL			  12
#define WANIFACE_NUM				  MAX_VC_NUM
#define PING_PUB_IP_ADDR			  "www.baidu.com"
#define CHECK_ROUTE_FILE			  "/proc/net/route"
#define ROLE_QUERY_FILE				  "/var/log/easymesh_log.txt"

typedef struct map_wan_type
{
	int address_type;
	int service_type;
	int forward_type; //0: bridge; 1:nat ip;2:nat pppoe
	int bridge_type;
	int ipv6_enable;
}MAP_WAN_TYPE;

#endif

#define MULTI_AP_SCRIPT "/var/multi_ap_script"
#define MULTI_AP_BACKHAUL_STA_CONFIG "/var/multi_ap_backhaul_sta.config"
#define MULTI_AP_BACKHAUL_BSS_CONFIG "/var/multi_ap_backhaul_bss.config"

#define MAP_EXT_DATA "/var/map_ext_data.conf"

typedef enum {
  MAP_FRONTHAUL_BSS = 0x20,
  MAP_BACKHAUL_BSS = 0x40,
  MAP_BACKHAUL_STA = 0x80
} MAP_BSS_TYPE_T;

typedef enum {
  MAP_DISABLED = 0,
  MAP_CONTROLLER,
  MAP_AGENT,
  MAP_AUTO,
  MAP_CONTROLLER_WFA = 129,
  MAP_AGENT_WFA = 130,
  MAP_CONTROLLER_WFA_R2 = 131,
  MAP_AGENT_WFA_R2 = 132,
  MAP_CONTROLLER_WFA_R3 = 133,
  MAP_AGENT_WFA_R3 = 134
} MAP_ROLE_TYPE_T;

struct config_info {
	unsigned char config_type;
	char *  ssid;
	char *  network_key;
	unsigned char network_type;
	unsigned char authentication_type;
	unsigned char bss_index;
	unsigned char encryption_type;
};

struct vlan_infos {
	int                enable_vlan;
	int                primary_vid;
	int                vssid_number;
	char **            vssids;
	int *              vids;
};

struct bss_info {
	char           ssid[33];
	char           *network_key_base64;
	char           network_key[64];
	char           *ssid_base64;
	unsigned char  network_type;
	unsigned char  is_enabled;
	unsigned char  encrypt_type;
	unsigned char  authentication_type;
	unsigned char  vid;
	unsigned char  bss_index;
	unsigned char  wsc_enc;
};
////////////////////////////////////////////////////////////////////////////////
struct radio_info {
	//per radio config data
	unsigned char    radio_type;
	unsigned char    radio_channel;
	unsigned char    radio_band_width;
	unsigned char    radio_control_sideband; // 0: upper, 1: lower
	char             repeater_ssid[33];
	char            *repeater_ssid_base64;
	unsigned char    bss_nr;
	struct bss_info *bss_data;
};

struct mib_info {
	unsigned char      reg_domain;
	unsigned int       customize_features;
	unsigned char      op_mode;
	unsigned char      map_role;
	char               device_name_buffer[MAX_MAP_DEVICE_NAME_LEN];
	unsigned char      map_configured_band;
	unsigned char      radio_nr;
#ifdef EASY_MESH_BACKHAUL
	unsigned char	   map_backhaul;
#endif
	struct radio_info *radio_data;
	struct vlan_infos   vlan_data;
};

//Used for customize feature, BIT0: customize M1/M2 info for CT ...
#define CUSTOMIZE_WSC_INFO		0x00000001  //Modify for AUTH/ENCRYPTION FLAGS in autconfig M1/M2 packet to fit CT SPEC
#define SUPPORT_OPEN_ENCRYPT	0x00000002  //multi-ap support open encrypt
#define SUPPORT_NOT_WPS_CYCLE	0x00000004  //multi-ap wps not cycle
#define SUPPORT_ETHERNET_ADAPTER	0x00000008  //multi-ap support ethernet adapter
#define SUPPORT_SHOW_EXTRA_TOPOLOGY	0x00000010  //multi-ap show extra topology
#define RCPI_ROAMING_ENABLE	0x00000080  // enable agent initiated rcpi roaming for ct

struct map_ext_conf{
	int    backhaul_switch;				// Whether backhaul link switch is enabled
	int    backhaul_radio_index;		// Current backhaul BSS/STA radio index, value:0(wlan0) 1(wlan1)
	int    check_interval;				// Time interval for easymesh check backhaul link, default 5s
	int    link_down_threshold;			// Threshold for easymesh judge whether current backhaul link is down physically
	int    discovery_down_threshold;	// Threshold for easymesh judge whether controller is lost on current backhaul while as physical connection is exist
	int    arp_loop;					// Whether arp loop detection is enabled
};

#define TLV_TYPE_WLAN_ACCESS_CONTROL      (1) // 0x01
#define TLV_TYPE_WLAN_SCHEDULE            (2) // 0x02

typedef enum {
  MAP_SYNC_WLAN_ACL_DATA      = 0x01,
  MAP_SYNC_WLAN_SCHEDULE_DATA = 0x02
} MAP_SYNC_DATA_TYPE_T;

struct acl_info {
	unsigned char acl_enable;
	unsigned char entry_num;
	unsigned char wlan_Idx;
	unsigned char vwlanIdx;
	unsigned char macAddr[MAC_ADDR_LEN];
};

struct wlan_schedule_info {
	unsigned char  wlan_Idx;
	unsigned char  enable;
	unsigned char  entry_num;
	unsigned char  mode;
	unsigned short eco;
	unsigned short fTime;
	unsigned short tTime;
	unsigned short day;
	unsigned int   inst_num;
};

struct vendor_specific_info {
	unsigned char  tlv_type;
	unsigned char  payload_nr;
	unsigned char  *payload;
	unsigned char  vendor_oui[3];
};

struct mib_specific_info {
	unsigned char 	             vendor_specific_nr;
	struct vendor_specific_info *vendor_specific_data;
};

static const uint8_t REALTEK_OUI[3]       = {0x00, 0xE0, 0x4C};

void map_get_ext_conf_data(struct map_ext_conf *map_ext_data, uint8_t        role);

void map_write_ext_conf_data(struct map_ext_conf *map_ext_data, char *file_path);

void rtk_stop_multi_ap_service();

int rtk_setup_multi_ap_block(unsigned char map_state);

int rtk_start_multi_ap_service();

uint8_t map_read_mib_to_config(struct mib_info *mib_data);

void map_fill_config_data(struct mib_info *mib_data, struct config_info **config_data, unsigned char *config_nr);

uint8_t map_update_config_file(struct mib_info *mib_data, struct mib_specific_info *mib_specific_data, char *config_file_path_from, char *config_file_path_to, uint8_t mode);

uint8_t map_write_mib_data(struct mib_info *mib_data, char *file_path);

void map_free_mib_data();

void map_write_to_config(FILE *fp, unsigned char config_nr, char **config_array);

void map_write_to_config_dec(FILE *fp, unsigned char config_nr, unsigned char *config_array);

void map_write_to_config_dec_int(FILE *fp, unsigned char config_nr, int *config_array);

#if defined(WLAN_MULTI_AP_ROLE_AUTO_SELECT)
int map_parse_easymesh_log_file(char *filename);

int map_link_parse_ping_file(char *filename);

int map_checkping(char *server1);

int map_getDefaultRoute(char *interface);

int map_is_controller_or_agent(void);
#endif

int rtk_setup_multi_ap_script(void);
int rtk_get_multi_ap_index_by_map_band(int band);
void map_restart_wlan(void);
#endif