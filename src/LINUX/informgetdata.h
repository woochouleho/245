#ifndef INCLUDE_INFORMGETDATA_H
#define INCLUDE_INFORMGETDATA_H

typedef struct StaInfo_s
{
	long StaOnlineTime;		//下挂终端在线时间，单位秒 
	char szStaName[32];	//下挂终端名称（如果有，则需提供）
	char pszStaType[16];		//下挂终端类型 （如果有，则需提供）pad，pc，mobile
	char szStaMac[6];		//下挂终端mac
	char szStaIP[16];			//下挂终端ip
	char szStaBrand[64];		//下挂终端品牌 （如果有，则需提供）Huawei，xiaomi，360
	char szStaOS[64];		//下挂终端操作系统 （如果有，则需提供）
	int StaWIFIIntensity; 	//下挂终端WIFI信号强度RSSI，单位dBm. 对于有线连接终端, 此参数值无意义.
	long StaSpeed; 		//下挂终端协商速率Speed, 单位Kbps 
	unsigned long long StaRecvTraffic; 	//下挂终端当次使用累计接收总流量,单位：Byte
	unsigned long long StaSendTraffic;	 	//下挂终端当次使用累计发送总流量,单位：Byte
	int status; 		//1：上线 0：下线 
	char szRadioType[8];  // "2.4G" ,"5G",对于有线连接终端, 此参数值无意义.
	char szWifiProtocol[16]; //wifi下挂终端的协议802.11a/802.11b/802.11g/802.11n/802.11ac/802.11ax String对于有线连接终端, 此参数值无意义
	int connectionType; 	//下挂终端和智能网关的连接形式（0：有线/1：⽆线）
	long usBandwidth; 		//表示下挂终端上行最大带宽，0表示不限，单位为kbps
	long dsBandwidth; 		//表示下挂终端下行最大带宽，0表示不限，单位为kbps
	long usGuaranteeBandwidth; 	//上行最小保证带宽，单位kbps .保证带宽未下发或下发为0，都表示不做保障 
	long dsGuaranteeBandwidth; 	//下行最小保证带宽，单位kbps .保证带宽未下发或下发为0，都表示不做保障 
	long upSpeed;		//下挂终端实时上行速率，单位kbps
	long downSpeed;	//下挂终端实时下行速率，单位kbps
}StaInfo;


typedef struct DeviceInfo_s
{
	char pppoe[64];  		//家庭网关PPPOE账号. 
	float ponTXPower; 		//发光功率dBm, 只对光上行设备需要填写.
	float ponRXPower; 		//接收光功率dBm, 只对光上行设备需要填写.
	char szDeviceMac[6];	//网关mac
	long onlineTime; 		//网关在线时间(单位：秒)
	unsigned long long recvTraffic; 		//网关当次使用累计接收总流量, BYTE
	unsigned long long sendTraffic; 		//网关当次使用累计发送总流量, BYTE
	unsigned long long sendSpeed; 		//实时上行速率，单位bps，计算500ms内两次流量差除以 500ms 的值 
	unsigned long long recvSpeed;		//实时下行速率，单位bps，计算500ms 内两次流量差除以500ms 的值 
	int cpuRate; 		// CPU使用率 0~100
	int memRate;		//内存使用率 0~100
	char szDeviceLanMac[4][6]; // 网关LAN侧Mac地址列表
	double ponTemperature; 		//光模块温度 摄氏度
	int StaInfoNum ;		//StaInfo数量
	StaInfo *pStaInfos; 		//网关下挂终端（包含下挂路由器）信息
}DeviceInfo;


int InformGetData_func(DeviceInfo **data);
int InformReleaseData_func(DeviceInfo *data);

#endif
