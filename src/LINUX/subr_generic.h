
#ifdef HANDLE_DEVICE_EVENT
int rtk_generic_pushbtn_reset_action(int action, int btn_test, int diff_time);
#endif

#ifdef CONFIG_CWMP_TR181_SUPPORT
#define WLAN_STATS_FOLDER			"/tmp/wlan_stats"
#define CHANNEL_CHANGE_COUNT_MANUAL		"chan_chg_manual_count"
#define CHANNEL_CHANGE_COUNT_AUTO_USER		"chan_chg_auto_user_count"
#define CHANNEL_CHANGE_COUNT_AUTO_DFS		"chan_chg_auto_dfs_count"
#define CHANNEL_CHANGE_COUNT_KNOWN		"chan_chg_unknown_count"

typedef enum {
	WALN_CHANGE_MANUAL = 0,
	WALN_CHANGE_AUTO_STARTUP,
	WALN_CHANGE_AUTO_USER,                                                                                                                               
	WALN_CHANGE_AUTO_Refresh,
	WALN_CHANGE_AUTO_DYNAMIC,
	WALN_CHANGE_AUTO_DFS,
	WALN_CHANGE_UNKNOWN,
} WLAN_CHANGE_REASON;

char rtk_generic_update_chanchg_reason_count(char *ifname, char reason, char write);
unsigned int rtk_generic_update_chanchg_time(char* ifname, char write);
#endif
