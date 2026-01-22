#ifndef WEB_LANGUAGE_DEFS_E8_H
#define WEB_LANGUAGE_DEFS_E8_H

#if defined(CONFIG_CMCC) || defined(CONFIG_CU_BASEON_CMCC)
#define E8B_START_STR		"1网关正在启动"
#define NO_CWMP_CONNECTION		"2无远程管理WAN连接"
#define CWMP_CONNECTION_DISABLE		"3远程管理WAN连接未生效"
#define CWMP_NO_DNS		"4无管理通道DNS信息"
#define CWMP_NO_ACSSETTING		"5无ACS配置参数"
#define ACSURL_GET_FAIL		"6ACS域名解析失败"
#else
#define E8B_START_STR		"家庭网关正在启动"
#define NO_CWMP_CONNECTION	"无远程管理 WAN 连接"
#define CWMP_CONNECTION_DISABLE	"远程管理 WAN 连接未生效"
#define CWMP_NO_DNS		"无管理通道 DNS 信息"
#define CWMP_NO_ACSSETTING	"无 ACS 配置参数"
#define ACSURL_GET_FAIL		"ACS 域名解析失败"
#endif

#endif
