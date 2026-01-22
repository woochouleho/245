
/* 
convert XML from rg 2 fc:
	lastgood.xml to config.xml
	lastgood_hs.xml to config_hs.xml

action before configD load_xml_file for save some key info


cases eg:

	1. REPLACE  "#"  => whole replace to fcname
	2. CONTINUE  =>  other action todo
	3. CHAIN_E  => replay for specail chain's element
	4. CHAIN  =>  chain name replace
	5. REBUILD => rebuild struct for some entry
		orig:
		  <Value Name="RING_CAD_OFF" Value="4000"/> <!--index=0-->
		  <Value Name="RING_CAD_OFF" Value="4000"/> <!--index=1-->
		  <Value Name="RING_CAD_OFF" Value="4000"/> <!--index=2-->
		  <Value Name="RING_CAD_OFF" Value="4000"/> <!--index=3-->
		  <Value Name="RING_CAD_OFF" Value="4000"/> <!--index=4-->
		  <Value Name="RING_CAD_OFF" Value="4000"/> <!--index=5-->
		  <Value Name="RING_CAD_OFF" Value="4000"/> <!--index=6-->
		  <Value Name="RING_CAD_OFF" Value="4000"/> <!--index=7-->
		after:
			<Value Name="RING_CAD_ON" Value="07d0,07d0,07d0,07d0,07d0,07d0,07d0,07d0"/>
	6. DELLINE  => delete the special line

*/
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <unistd.h>

#define LASTGOOD "/var/config/lastgood.xml"
#define LASTGOOD_HS "/var/config/lastgood_hs.xml"
#define CONFIG "/var/config/config.xml"
#define CONFIG_HS "/var/config/config_hs.xml"
//#define LASTGOOD "lastgood.xml"
//#define CONFIG "config1.xml"

#define REPLACE 0x1
#define CHAIN_E 0x2
#define CHAIN 0x4
#define DELLINE 0x8
#define CONTINUE 0x10
#define REBUILD 0x20

#define CSTYPE 0x1
#define HSTYPE 0x2

enum special_chain{
	enum_null,
	enum_voip,
	enum_end
};

struct mapping_name{
	unsigned char rgname[256];
	unsigned char fcname[256];
	int flag;
	char separ[64];
	int cfgtype;
	int chain_e_type;
}rg2fc_mapping[]={
	{"</Dir>","</chain>\n",REPLACE,"#", CSTYPE|HSTYPE, enum_null}, //fix here
	//{"<!--index","",DELLINE,"<Value Name", CSTYPE}, // fix here
	{"<Config Name=\"ROOT\">","<Config_Information_File_8671>\n",REPLACE,"#", CSTYPE, enum_null},
	{"<Config Name=\"ROOT\">","<Config_Information_File_HS>\n",REPLACE,"#", HSTYPE, enum_null},
	{"</Config>","</Config_Information_File_8671>\n",REPLACE,"#", CSTYPE, enum_null},
	{"</Config>","</Config_Information_File_HS>\n",REPLACE,"#", HSTYPE, enum_null},
	{"<Dir Name=\"MIB_TABLE\">","",DELLINE,"#", CSTYPE, enum_null},
	{"<Dir Name=\"HW_MIB_TABLE\">","",DELLINE,"#", HSTYPE, enum_null},
	{"<Dir Name","<chain chainName",REPLACE|CONTINUE,"=", CSTYPE|HSTYPE, enum_null},
	
	{"REG_RETRY","PROXY_REG_RETRY",CHAIN_E,"\"", CSTYPE, enum_voip},
	{"SESSION_UPDATE_TIMER","PROXY_SESSION_EXPIRE",CHAIN_E,"\"", CSTYPE, enum_voip},
	{"SIPTLS_ENABLE","TLS_ENABLE",CHAIN_E,"\"", CSTYPE, enum_voip},
	{"OUTBOUND_PORT","PROXY_OUTBOUND_PORT",CHAIN_E,"\"", CSTYPE, enum_voip},
	{"OUTBOUND_ADDR","PROXY_OUTBOUND_ADDR",CHAIN_E,"\"", CSTYPE, enum_voip},
	{"OUTBOUND_ENABLE","PROXY_OUTBOUND_ENABLE",CHAIN_E,"\"", CSTYPE, enum_voip},
	{"REG_EXPIRE","PROXY_REG_EXPIRE",CHAIN_E,"\"", CSTYPE, enum_voip},
	{"DOMAIN_NAME","PROXY_DOMAIN_NAME",CHAIN_E,"\"", CSTYPE, enum_voip},
	{"REGISTRARSERVER_PORT","PROXY_REGISTRARSERVER_PORT",CHAIN_E,"\"", CSTYPE, enum_voip},
	{"REGISTRARSERVER_ADDR","PROXY_REGISTRARSERVER_ADDR",CHAIN_E,"\"", CSTYPE, enum_voip},
	{"PORT","PROXY_PORT",CHAIN_E,"\"", CSTYPE, enum_voip},
	{"ADDR","PROXY_ADDR",CHAIN_E,"\"", CSTYPE, enum_voip},
	{"ENABLE","PROXY_ENABLE",CHAIN_E,"\"", CSTYPE, enum_voip},
	{"URI","PROXY_URI",CHAIN_E,"\"", CSTYPE, enum_voip},
	{"PASSWORD","PROXY_PASSWORD",CHAIN_E,"\"", CSTYPE, enum_voip},
	{"LOGIN_ID","PROXY_LOGIN_ID",CHAIN_E,"\"", CSTYPE, enum_voip},
	{"NUMBER","PROXY_NUMBER",CHAIN_E,"\"", CSTYPE, enum_voip},
	{"DISPLAY_NAME","PROXY_DISPLAY_NAME",CHAIN_E,"\"", CSTYPE, enum_voip},
	
	{"LED_INDICATOR_TIMER_TBL","LED_INDICATOR_TIMER",CHAIN,"\"", CSTYPE, enum_null},
	{"VoiceService","VOIP_TBL",CHAIN,"\"", CSTYPE, enum_null},

	{"RING_CAD_OFF","4", REBUILD, "", CSTYPE, enum_null},
	{"RING_CAD_ON","4", REBUILD, "", CSTYPE, enum_null},
	{"RING_CADENCE_USE","2", REBUILD, "", CSTYPE, enum_null},
	{"RING_PHONE_NUM","8", REBUILD, "", CSTYPE, enum_null},
	{"PRECEDENCE","2", REBUILD, "", CSTYPE, enum_null},
	{"FRAME_SIZE","2", REBUILD, "", CSTYPE, enum_null},
	{}
};

FILE * cs_g =NULL;
struct multi_index_2_string{
	unsigned char name[32];
	unsigned char value[256];
	int index;
}record={{0},{0},0};

void record_value(unsigned char *line, int len)
{
	//<Value Name="RING_CAD_OFF" Value="4000"/> <!--index=0-->
	unsigned char *ptr=NULL;
	unsigned char *ptr_i=NULL;
	static int offset = 0;

	ptr=strtok(line, "\"");
	ptr=strtok(NULL, "\"");
	if(record.name[0] == 0){
		sprintf(record.name, "%s", ptr);
		offset = 0;
	}
	else if(strcmp(record.name, ptr))
		printf("[Convert] rebuild maybe wrong!\n");
	ptr=strtok(NULL, "\"");
	ptr=strtok(NULL, "\"");
	if(len == 4)
		offset += sprintf(record.value+offset, "%04x,", atoi(ptr));
	else if(len == 2)
		offset += sprintf(record.value+offset, "%02x,", atoi(ptr));
	else if(len == 8)
		offset += sprintf(record.value+offset, "%08x,", atoi(ptr));
	ptr=strtok(NULL, "\"");
	ptr_i=strtok(ptr, "=");
	ptr_i=strtok(NULL, "=");
	record.index = atoi(ptr_i);

	return;
}

void do_convert_4_eachline(unsigned char * line_org,unsigned char *newline, int cfgtype)
{
	unsigned char* substr=NULL;
	static int filechange = 0;
	static int delfirstDir = 0;
	int off = 0;
	static int start_rebuild = 0;
	static int preRebuild_matchindex = -1;

	static int flag = 0;
	unsigned char* strp=NULL;
	static int special_chain_start = 0;
	static int special_chain_type = 0;
	int i = 0;
	unsigned char tmpline[1024]={0};
	unsigned char * line = line_org;

	if(filechange != cfgtype){
		filechange = cfgtype;
		delfirstDir = 1;
	}

	while(*line ==' ') line++;

	/* for chain_e type*/
	if(strstr(line, "<Dir Name=\"PROXIES\">")){
		special_chain_start = 1;
		special_chain_type = enum_voip;
	}


	for(i = 0; i< sizeof(rg2fc_mapping)/sizeof(rg2fc_mapping[0]); i++)
	{
		if((cfgtype & (rg2fc_mapping[i].cfgtype)) == 0)
			continue;

		substr = strstr(line, rg2fc_mapping[i].rgname);
		if(substr){
			if(start_rebuild == 1 && preRebuild_matchindex != i)
			{
				start_rebuild = 0;  //rebuild end
				//write a new line to file
				record.value[strlen(record.value)-1]='\0';
				fprintf(cs_g, "<Value Name=\"%s\" Value=\"%s\"/>\n", record.name, record.value);
				memset(&record, 0, sizeof(record));
				preRebuild_matchindex = -1;
			}

			if(delfirstDir == 1 && i == 0){
				newline[0]='\0';
				delfirstDir = 0;
				return;
			}
			else if((rg2fc_mapping[i].flag) & REPLACE && rg2fc_mapping[i].separ[0]=='#'){ // whole tihuan
				snprintf(newline,1024,"%s", rg2fc_mapping[i].fcname);
				if(special_chain_start==1 && i == 0){
					special_chain_start = 0;
					special_chain_type = 0;
				}
				return;
			}
			else if((rg2fc_mapping[i].flag) & DELLINE && rg2fc_mapping[i].separ[0] == '#'){
					newline[0]='\0';
					return;
			}
			else if((rg2fc_mapping[i].flag) & REBUILD ){
				if(start_rebuild == 0 && preRebuild_matchindex == -1){
					start_rebuild = 1;
					preRebuild_matchindex = i;
				}
				if(start_rebuild){
					newline[0]='\0';
					record_value(line, atoi(rg2fc_mapping[i].fcname));
					return;
				}
			}
			//<Value Name="RING_CAD_OFF" Value="4000"/> <!--index=0-->
			//<chain chainName="VOIP_TBL">
			else if((rg2fc_mapping[i].flag) & REPLACE && rg2fc_mapping[i].separ[0]!='#'){
				strp = strstr(line, rg2fc_mapping[i].separ);
				snprintf(newline, 1024,"%s%s", rg2fc_mapping[i].fcname, strp);
				if((rg2fc_mapping[i].flag) & CONTINUE){
					sprintf(line, "%s",newline); //update line to continue
					continue;
				}
				else
					return;
			}

			if((((rg2fc_mapping[i].flag) & CHAIN_E) && (special_chain_start == 1) && (special_chain_type == rg2fc_mapping[i].chain_e_type)) \
				 || (rg2fc_mapping[i].flag) & CHAIN){
				strp = strstr(line, rg2fc_mapping[i].rgname);
				if(strp)
				{
					*strp = '\0';
					snprintf(newline, 1024, "%s%s%s",line, rg2fc_mapping[i].fcname, strp+strlen(rg2fc_mapping[i].rgname));
				}
				return;
			}
					
		}
	}
	snprintf(newline, 1024, "%s" ,line);
	return;
}

int main()
{
	FILE *lastfile=NULL;
	FILE *configfile=NULL;
	FILE *lastfile_hs=NULL;
	FILE *configfile_hs=NULL;
	unsigned char buf[1024] = {0};
	unsigned char cmd[1024] = {0};
	unsigned char write_line[1024]={0};
	
	if( access(CONFIG, F_OK ) == 0 ) {
		printf("[Convert] config.xml exist! Do nothing!\n");
		return 0;
	}

	lastfile = fopen(LASTGOOD, "r");
	if(!lastfile){
		printf("[Convert] lastgood.xml fopen fail! Do nothing\n");
		return 0;
	}

	printf("[Convert CS] ======Start======\n");

	//=============================CS=====================
	configfile = fopen(CONFIG, "w");
	if(!configfile){
		printf("[Convert] config.xml fopen fail!\n");
		return 0;
	}

	cs_g = configfile;  //for rebuild

	while(fgets(buf, sizeof(buf),lastfile) != NULL)
	{
		memset(write_line,0, sizeof(write_line));
		do_convert_4_eachline(buf, write_line, CSTYPE);
		fprintf(configfile, "%s", write_line);
	}
	
	fclose(lastfile);
	fclose(configfile);

	snprintf(cmd,1024, "dos2unix %s", CONFIG);
	system(cmd);
	printf("[Convert CS] ======SUCCESS!======\n");
	//=============================HS=====================
	printf("[Convert HS] ======Start======\n");
	if( access(CONFIG_HS, F_OK ) == 0 ) {
		printf("[Convert] config_hs.xml exist! Do nothing!\n");
		return 0;
	}

	lastfile_hs = fopen(LASTGOOD_HS, "r");
	if(!lastfile_hs){
		printf("[Convert] lastgood_hs.xml fopen fail! Do nothing\n");
		return 0;
	}

	configfile_hs = fopen(CONFIG_HS, "w");
	if(!configfile_hs){
		printf("[Convert] config_hs.xml fopen fail!\n");
		return 0;
	}

	while(fgets(buf, sizeof(buf),lastfile_hs) != NULL)
	{
		memset(write_line,0, sizeof(write_line));
		do_convert_4_eachline(buf, write_line, HSTYPE);
		fprintf(configfile_hs, "%s", write_line);
	}
	
	fclose(lastfile_hs);
	fclose(configfile_hs);

	snprintf(cmd,1024, "dos2unix %s", CONFIG_HS);
	system(cmd);

	printf("[Convert HS] ======SUCCESS!======\n");
	//====================================================

	system("cp /var/config/config.xml /var/config/config_org.xml");
	system("cp /var/config/config_hs.xml /var/config/config_hs_org.xml");
	system("rm -rf /var/config/lastgood.xml_bak /var/config/lastgood_hs.xml_bak");
	system("mv /var/config/lastgood.xml /var/config/lastgood.xml_org");
	system("mv /var/config/lastgood_hs.xml /var/config/lastgood_hs.xml_org");
	
	return 0;
}
