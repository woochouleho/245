#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/file.h>
#include "utility.h"
#include "mib.h"
#include "debug.h"
#include <sys/sysinfo.h>

#if 0
#define DEBUGP	printf
#else
#define DEBUGP(format, args...)
#endif

#define DPRINT(FMT, args...) do{ fprintf(stderr, FMT, ##args); }while(0)


#define error -1
#define chain_record_number	(MIB_CHAIN_TBL_END - CHAIN_ENTRY_TBL_ID + 1)
#define MAX_OBJ	8

// xml string buffer length
#define OUT_STR_BUF_LEN (65534)
// data buffer for xml string
static char strbuf[OUT_STR_BUF_LEN+1] = {0};
//data count and buffer index
static int bc = 0;
int xor_idx = 0;
static void xml_write(FILE *fp, int *xor_idx);

struct chain_obj_s {
	char obj_name[32];
	int obj_per_size;
	int obj_max_num;
	int cur_num;
};

static unsigned char load_to_configd;
static const char empty_str[] = "";
static char LINE[MAX_MIB_VAL_STR_LEN];
static char chain_updated[chain_record_number];

typedef struct chainNode {
    int id;
	unsigned char *chainEntry;
    struct chainNode * next;
} chainNode_t;

//Adding an item to the end of the list
static void push_chainNode(chainNode_t * head, int id, unsigned char *chainEntry) {
    chainNode_t * current = head;

	while (current->next != NULL) {
		current = current->next;
	}

	current->next = (chainNode_t*)calloc(1, sizeof(chainNode_t));
	current->next->id = id;
	current->next->chainEntry = chainEntry;
	current->next->next = NULL;
}

static int pop_chainNode(chainNode_t ** head) 
{
    int retval = -1;
    chainNode_t * next_node = NULL;

    if (*head == NULL) return -1;

    next_node = (*head)->next;
    retval = (*head)->id;
    free(*head);
    *head = next_node;

    return retval;
}

//the function will allocate memory for idtbl table, we need to free memory when call get_chainNodeId()
static void get_chainNodeId(chainNode_t * head, int** idtbl, int* len)
{
	int i = 0;
	chainNode_t * current = head;
	int * info = NULL;
	int num= 0, find = 0;
	int currentSize = 100;

	if(head == NULL) return;

	info = calloc(1, currentSize*sizeof(int));
	while (current != NULL) {
		find = 0;
		for(i=0; i < num; i++){
			if(info[i]==current->id){
				find = 1;
				break;
			}
		}
		if(find==0) {
			//if num is larger than currentSize, we assign large size array for info table
			if(num >= currentSize-1) { 
				currentSize = currentSize + 100;
				info = realloc(info, currentSize);
			}
			info[num] = current->id;
			num++;
		}
        current = current->next;
    }
	*idtbl = info;
	*len = num;
}

static inline int MIB_SET(int id, void *value) 
{
	if (load_to_configd)
		return mib_set(id, value);
	else
		return _mib_set(id, value);
}

static inline void MIB_CHAIN_CLEAR(int id)
{
	if (load_to_configd)
		mib_chain_clear(id);
	else
		_mib_chain_clear(id);
}

static inline int MIB_CHAIN_ADD(int id, void *ptr)
{
	if (load_to_configd)
		return mib_chain_add(id, ptr);
	else
		return _mib_chain_add(id, ptr);
}

static inline int MIB_CHAIN_UPDATE(int id, void *ptr, unsigned int recordNum)
{
	if (load_to_configd)
		return mib_chain_update(id, ptr, recordNum);
	else
		return _mib_chain_update(id, ptr, recordNum);
}

static inline int MIB_CHAIN_GET_LOAD(int id, unsigned int recordNum, void *chainEntry, int size)
{
	int ret = 0;
	void *tmpchainEntry;
	
	if (load_to_configd) {
		ret = mib_chain_get(id, recordNum, chainEntry);
	}
	else{
		tmpchainEntry = _mib_chain_get(id, recordNum);
		if(tmpchainEntry == NULL){
			ret=0; //fail
		}
		else{
			memcpy(chainEntry, tmpchainEntry, size);
			ret=1;
		}
	}

	return ret;
}

static char *get_line(FILE *fp, char *s, int size)
{
	char *pstr;
	int len;

	while (1) {
		if (!fgets(s, size, fp) || !(len = strlen(s)) || s[len-1] != '\n')
			return NULL;
		pstr = trim_white_space(s);
		if (strlen(pstr))
			break;
	}

	return pstr;
}

static int table_setting(char *line, CONFIG_DATA_T cnf_type)
{
	int i;
	char *pname, *pvalue;
	char *first, *last;
	unsigned char mibvalue[MAX_MIB_VAL_STR_LEN];

	// get name
	first = strchr(line, '"');
	if(first==NULL)
		return error;
	pname = first+1;
	last = strchr(pname, '"');
	if(last==NULL)
		return error;
	*last = '\0';
	DEBUGP("table name=%-25s", pname);

	for (i = 0; mib_table[i].id; i++) {
		if (mib_table[i].mib_type != cnf_type)
			continue;

		if (!strcmp(mib_table[i].name, pname))
			break;
	}

	if (mib_table[i].id == 0) {
		TRACE(STA_WARNING, "%s: Invalid table entry name: %s\n", __FUNCTION__, pname);
		//return error;
		return 0;
	}

	// get value
	first = strchr(last+1, '"');
	if(first==NULL)
		return error;
	pvalue = first+1;
	last = strrchr(pvalue, '"');
	if (last == NULL)
		pvalue = (char *)empty_str;
	else
		*last = '\0';

	DEBUGP("value=%s\n", pvalue);

	if (string_to_mib_ex(mibvalue, pvalue, mib_table[i].type, mib_table[i].size, 1))
		return error;

	if (!MIB_SET(mib_table[i].id, mibvalue)) {
		DPRINT("Set MIB[%s] error!\n", mib_table[i].name);
		return error;
	}

	return 0;
}

/* Get object info.
 * obj_info: object info list
 * desc: descriptor of this object
 * index: return the index of obj_info for this object.
 * Return:
 *	0: successful
 * 	-1: error
 */
static int get_object_info(struct chain_obj_s *obj_info, mib_chain_member_entry_T * desc, int *index)
{
	int found, i, k,len;
	mib_chain_member_entry_T * obj_desc;

	if (desc->record_desc == NULL)
		return error;

	// find object info.
	found = 0;
	for (k = 0; k < MAX_OBJ; k++) {
		if (obj_info[k].obj_name[0] == '\0')
			break;

		if (!strncmp(obj_info[k].obj_name, desc->name, sizeof(obj_info[k].obj_name))) {
			found = 1;
			break;
		}
	}

	if (k == MAX_OBJ) {
		DPRINT("%s: data overflow !\n", __FUNCTION__);
		k = 0;
	}

	if (!found) {		// create one
		len = sizeof(obj_info[k].obj_name);
		memset(obj_info[k].obj_name, 0, len );
		strncpy(obj_info[k].obj_name, desc->name, len-1);

		obj_desc = desc->record_desc;

		obj_info[k].obj_per_size = 0;
		for (i = 0; obj_desc[i].name[0]; i++) {
			obj_info[k].obj_per_size += obj_desc[i].size;
		}

		/* max entry number of this chain_obj */
		obj_info[k].obj_max_num = desc->size/obj_info[k].obj_per_size;
		obj_info[k].cur_num = 0;
	}
	//DPRINT("%s(%d) name=%s, size=%d, persize=%d, max_num=%d, cur_num=%d\n", __FUNCTION__, __LINE__, desc->name, desc->size, obj_info[k].obj_per_size, obj_info[k].obj_max_num, obj_info[k].cur_num);
	*index = k;

	return 0;
}

/*
 *	Parsing invalid Chain tag, consume and ignore the whole chain.
 * Return:
 *	-1 : error
 *	 0 : successful
 *	 1 : empty chain(object)
 */
static int skip_value_object(FILE *fp, mib_chain_member_entry_T *root_desc)
{
	char *pstr, *pname;
	mib_chain_member_entry_T root_desc_fake;
	int len;

	while (!feof(fp)) {
		pstr = get_line(fp, LINE, sizeof(LINE));

		if (!pstr)
			return 1;

		if (!strncmp(pstr, "</chain", 7)) {
			DEBUGP("End of obj %s\n\n", root_desc->name);
			break;	// end of chain object
		}

		// check OBJECT_T
		if (!strncmp(pstr, "<chain", 6)) {
			// get Object name
			strtok(pstr, "\"");
			pname = strtok(NULL, "\"");
			DEBUGP("obj_name=%s (skipped)\n", pname);

			if (pname == NULL) {
				DPRINT("[%s %d] not existed !\n", __FUNCTION__, __LINE__);
				continue;
			}
			len = sizeof(root_desc_fake.name);
			memset(root_desc_fake.name, 0, len);
			strncpy(root_desc_fake.name, pname, len-1);
			skip_value_object(fp, &root_desc_fake);
		}
		//else {
		else if (!strncmp(pstr, "<Value", 6)) {
			// get name
			strtok(pstr, "\"");
			pname = strtok(NULL, "\"");
			DEBUGP("name=%s\n", pname);

			if (pname == NULL) {
				DPRINT("[%s %d] not existed !\n", __FUNCTION__, __LINE__);
				continue;
			}
		}
	}

	return 0;
}

/*
 * Return:
 *	-1 : error
 *	 0 : successful
 *	 1 : empty chain(object)
 */
static int put_value_object(FILE *fp, unsigned char *entry, mib_chain_member_entry_T * root_desc, mib_chain_member_entry_T * this_desc)
{
	int i, empty_chain, ret, index,len;
	char *pstr;
	struct chain_obj_s object_info[MAX_OBJ];
	char *pname, *pvalue;
	char *first, *last;
	char string[MAX_MIB_VAL_STR_LEN];
	char tmp[MAX_MIB_VAL_STR_LEN];
	mib_chain_member_entry_T root_desc_fake;

	memset(object_info, 0, sizeof(object_info));

	empty_chain = 1;

	while (!feof(fp)) {
		pstr = get_line(fp, LINE, sizeof(LINE));

		if (!pstr)
			return empty_chain;

		if (!strncmp(pstr, "</chain", 7)) {
			DEBUGP("End of obj %s\n\n", root_desc->name);
			break;	// end of chain object
		}

		//if (this_desc == NULL)
		//	continue;

		// check OBJECT_T
		if (!strncmp(pstr, "<chain", 6)) {
			// get Object name
			strtok(pstr, "\"");
			pname = strtok(NULL, "\"");
			DEBUGP("obj_name=%s\n", pname);

			if (pname == NULL) {
				DPRINT("[%s %d] not existed !\n", __FUNCTION__, __LINE__);
				continue;
				//return 0;
			}

			for (i = 0; this_desc[i].name[0]; i++) {
				if (!strcmp(this_desc[i].name, pname))
					break;
			}

			if (this_desc[i].name[0] == '\0') {
				DPRINT("%s: Chain Object %s member %s descriptor not found ! Skip it ...\n", __FUNCTION__, root_desc->name, pname);
				len = sizeof(root_desc_fake.name);
				memset(root_desc_fake.name, 0, len );
				strncpy(root_desc_fake.name, pname, len-1);
				empty_chain = skip_value_object(fp, &root_desc_fake);
				continue;
				//return error;
			}

			// get object info.
			ret = get_object_info(object_info, &this_desc[i], &index);
			if (ret)
				return error;

			if (object_info[index].cur_num < object_info[index].obj_max_num) {
				ret = put_value_object(fp, entry + this_desc[i].offset + object_info[index].cur_num * object_info[index].obj_per_size,
					     &this_desc[i], this_desc[i].record_desc);
				if (ret == error)
					return error;
			}
			else {
				DPRINT("Chain %s member %s entry %d exceeds max_num %d, skip it ...\n", root_desc->name, pname, object_info[index].cur_num+1, object_info[index].obj_max_num);
				strncpy(root_desc_fake.name, pname, sizeof(root_desc_fake.name)-1);
				skip_value_object(fp, &root_desc_fake);
				ret = 1; /* entry number exceeded, stop appending to avoid memory overflow */
			}
			object_info[index].cur_num++;
		}
		//else {
		else if (!strncmp(pstr, "<Value", 6)) {
			// get name
			first = strchr(pstr, '"');
			if(first==NULL)
				continue;
			pname = first+1;
			last = strchr(pname, '"');
			if(last==NULL)
				continue;
			*last = '\0';

			for (i = 0; this_desc[i].name[0]; i++) {
				if (!strcmp(this_desc[i].name, pname))
					break;
			}

			if (this_desc[i].name[0] == '\0') {
				DPRINT("Chain %s member %s not found !\n", root_desc->name, pname);
				continue;
				//return error;
			}
			DEBUGP("name=%-25s", pname);

			// get value
			first = strchr(last+1, '"');
			if(first==NULL)
				continue;
			pvalue = first+1;
			last = strrchr(pvalue, '"');
			if (last == NULL)
				pvalue = (char *)empty_str;
			else
				*last = '\0';
			DEBUGP("value=%s\n", pvalue);
			sprintf(string, "%s", pvalue);

#ifdef CONFIG_USER_XML_STRING_ENCRYPTION
			if(this_desc[i].type == ENCRYPT_STRING_T){
				sprintf(tmp, "%s", string);
				rtk_xmlfile_str_decrypt(tmp, string);
			}
#endif

			// put value
			ret = string_to_mib(entry + this_desc[i].offset, string, this_desc[i].type, this_desc[i].size);
			if (ret == error) {
				DPRINT("%s: Invalid chain member ! (name=\"%s\", value=\"%s\")\n", __FUNCTION__, pname, string);
				continue;
				//return error;
			}
		}
		empty_chain = 0;
	}

	return empty_chain;
}

static int chain_setting(FILE *fp, char *line, CONFIG_DATA_T cnf_type, chainNode_t * head)
{
	int i, empty_chain,len, index=-1;
	char *ptoken;
	char *pindex;
	mib_chain_member_entry_T *rec_desc;
	mib_chain_member_entry_T root_desc;
	unsigned char *chainEntry;
	char *saveptr = NULL;

	// get chain name
	strtok_r(line, "\"", &saveptr);
	ptoken = strtok_r(NULL, "\"", &saveptr);
	DEBUGP("Chain name=%s\n", ptoken);

	//get chain index
	pindex = strtok_r(NULL, "=", &saveptr);
	if(pindex!=NULL){
		pindex = strtok_r(NULL, "-", &saveptr);
		if(pindex!=NULL){
			index = atoi(pindex);
			DEBUGP("index=%d\n", index);
		}
	}

	// get chain info
	for (i = 0; mib_chain_record_table[i].id; i++) {
		if (mib_chain_record_table[i].mib_type != cnf_type)
			continue;

		if (!strcmp(mib_chain_record_table[i].name, ptoken))
			break;
	}

	if (mib_chain_record_table[i].id == 0) {
		DPRINT("%s: Invalid chain entry name: %s -- skip it ...\n", __FUNCTION__, ptoken);
		len= sizeof(root_desc.name);
		memset(root_desc.name, 0, len );
		strncpy(root_desc.name, ptoken, len-1);
		empty_chain = skip_value_object(fp, &root_desc);
		return 1;
	}

	// get chain descriptor
	rec_desc = mib_chain_record_table[i].record_desc;

	//clear orginal record when bootup
	if(access("/tmp/config_updated", F_OK)!=0){
		if (chain_updated[mib_chain_record_table[i].id - CHAIN_ENTRY_TBL_ID] == 0) {
			MIB_CHAIN_CLEAR(mib_chain_record_table[i].id);	//clear chain record
			chain_updated[mib_chain_record_table[i].id - CHAIN_ENTRY_TBL_ID] = 1;
		}
	}
	len = sizeof(root_desc.name);
	memset(root_desc.name, 0, len);
	strncpy(root_desc.name, mib_chain_record_table[i].name, len-1);
	chainEntry = calloc(1, mib_chain_record_table[i].per_record_size);

	//we find the chain entry when index is indicated
	if(access("/tmp/config_updated", F_OK)==0 && index != -1){
		MIB_CHAIN_GET_LOAD(mib_chain_record_table[i].id, index, chainEntry, mib_chain_record_table[i].per_record_size);
	}
	empty_chain = put_value_object(fp, chainEntry, &root_desc, rec_desc);

	/*collect chain entry and update chain entry when finish parsing config file by update_chain_setting()*/
	if (empty_chain == 0 || index != -1) {
		if(head->chainEntry== NULL){//initialize linked list
	    	head->id = mib_chain_record_table[i].id;
			head->chainEntry = chainEntry;
	    	head->next = NULL;
		}
		else{//push chain information into linked list
			push_chainNode(head, mib_chain_record_table[i].id, chainEntry);
		}
	}
	else {
		DEBUGP("Empty Chain.\n");
		MIB_CHAIN_CLEAR(mib_chain_record_table[i].id);
		if(chainEntry)
			free(chainEntry);
	}

	return empty_chain;
}

static int update_setting(FILE *fp, char *line, CONFIG_DATA_T cnf_type, chainNode_t * head, CONFIG_MIB_T action)
{
	int ret = 0;

	if (!strncmp(line, "<Value", 6)){
		if(action == CONFIG_MIB_ALL || action == CONFIG_MIB_TABLE)
			ret = table_setting(line, cnf_type);
	}else if (!strncmp(line, "<chain", 6)){
		if(action == CONFIG_MIB_ALL || action == CONFIG_MIB_CHAIN)
			ret = chain_setting(fp, line, cnf_type, head);
		else{
			// ignore the whole chain
			DEBUGP("skip %s\n", line);
			mib_chain_member_entry_T root_desc_fake;
			skip_value_object(fp, &root_desc_fake);
		}
	}else {
		DPRINT("Unknown statement: %s\n", line);
		ret = error;
	}

	return ret;
}

static void update_chain_setting(CONFIG_DATA_T cnf_type, chainNode_t * head)
{
	int size = 0;
	int *idtbl = NULL;
	int i=0;
	
	/*flush modify chain table*/
	get_chainNodeId(head, &idtbl, &size);
	for(i=0; i < size; i++){
		MIB_CHAIN_CLEAR(idtbl[i]);
	}
	if(idtbl != NULL) free(idtbl);
	
	/*add modified chain entry back and pop in the linked list*/
	chainNode_t * current = head;
    while (current != NULL) {
		MIB_CHAIN_ADD(current->id, current->chainEntry);
		/*chainEntry is allocated by chain_setting(), remeber to free memory*/
		free(current->chainEntry);
        pop_chainNode(&current);
    }
	head = NULL;
}

/*
 *	Return:
 *	-1 on error
 *	config type of CONFIG_DATA_T on success
 */
int _load_xml_file(const char *loadfile, CONFIG_DATA_T cnf_type, unsigned char flag, CONFIG_MIB_T action)
{
	int ret = 0;
	char *pstr;
	FILE *fp = NULL;
	chainNode_t *head=NULL;
	char cmd[512];
	int isEnd=0;
	char *conf_file=(char *)loadfile;
	CONFIG_DATA_T dtype;
//#ifdef XOR_ENCRYPT
	char tempFile[32]={0};
//#endif
	
	load_to_configd = flag;
	
	head = (chainNode_t*)calloc(1, sizeof(chainNode_t));
	
#ifdef CONFIG_USER_CONF_ON_XMLFILE
	if (strcmp(CONF_ON_XMLFILE_HS, loadfile) ==0 || strcmp(CONF_ON_XMLFILE, loadfile) ==0){
		if(checkConfigFile(loadfile, 1)==0){
			if (strcmp(CONF_ON_XMLFILE_HS, loadfile) ==0){ //CONF_ON_XMLFILE_HS invalid , /var/config/config_hs.xml is not exist
				if (checkConfigFile("/var/config_static/config_hs.xml", 1) != 0){
					snprintf(cmd, sizeof(cmd), "cp -rf /var/config_static/config_hs.xml %s", CONF_ON_XMLFILE_HS);
					system(cmd); //overwrite the original config_hs.xml
				}	
			}
#ifdef CONFIG_USER_XML_BACKUP
			if (strcmp(CONF_ON_XMLFILE, loadfile) ==0){ //CONF_ON_XMLFILE invalid
				if (checkConfigFile(CONF_ON_XMLFILE_BAK, 1) != 0){
					snprintf(cmd, sizeof(cmd), "cp -rf %s %s", CONF_ON_XMLFILE_BAK, loadfile);
					system(cmd); //overwrite the original config.xml
				}	
			}
#endif
		}
	}
#endif

//#ifdef XOR_ENCRYPT
	if(check_xor_encrypt((char*)loadfile)==1){
		/* decrypt to plain text config file */
		conf_file = tempFile;
		xor_encrypt((char*)loadfile, tempFile);
	}
//#endif

	if (!(dtype = (CONFIG_DATA_T)checkConfigFile(conf_file, 0)) ||
		(cnf_type != UNKNOWN_SETTING && dtype != cnf_type)) {
		DPRINT("Invalid config file: %s!\n", conf_file);
		ret = error;
		goto ret;
	}
	ret = (int)dtype;

	if (!(fp = fopen(conf_file, "r"))) {
		DPRINT("User configuration file can not be opened: %s\n", conf_file);
		ret = error;
		free(head);
		goto ret;
	}

	flock(fileno(fp), LOCK_SH);

	pstr = get_line(fp, LINE, sizeof(LINE));
	if(pstr==NULL){
		ret = error;
		free(head);
		flock(fileno(fp), LOCK_UN);
		goto ret;
	}

	/* initialize chain update flags on bootup */
	if(access("/tmp/config_updated", F_OK)!=0){
	memset(chain_updated, 0, sizeof(chain_updated));
	}

	while (!feof(fp)) {
		pstr = get_line(fp, LINE, sizeof(LINE));	//get one line from the file
		if(pstr != NULL){
			if(strlen(pstr) > 0){
				if (!strcmp(pstr, CONFIG_TRAILER)
				    || !strcmp(pstr, CONFIG_TRAILER_HS)) {
				    isEnd=1;
					break;	// end of configuration
				}

				if (update_setting(fp, pstr, dtype, head, action) < 0) {
					DPRINT("update setting fail!\n");
					ret = error;
					break;
				}
			}
		}
	}
	
	if(isEnd == 0){
		ret = error;
	}
	/* add the config file chain entry back, we only add chain entry with index*/
	if(ret != error){
		update_chain_setting(dtype, head);
	}
	
	flock(fileno(fp), LOCK_UN);

	/* No errors */
	if (load_to_configd && ret != error) {
		if (mib_update_ex(dtype, CONFIG_MIB_ALL, 1) == 0)
			ret = error;
	}

	/*touch /tmp/config_updated file to indicate boot or not*/
	system("echo 1 > /tmp/config_updated");
ret:
	if(fp) fclose(fp);
//#ifdef XOR_ENCRYPT
	if (strlen(tempFile))
		unlink(tempFile);
//#endif
	return ret;
}

#define TAB_DEPTH(fp, x) 		\
do {					\
	int i;				\
	for (i = 0; i < x; i++)		\
		bc += snprintf(strbuf+bc, OUT_STR_BUF_LEN-bc, " ");	\
} while (0)

static unsigned char save_from_configd;
static char *FORMAT_STR = "<Value Name=\"%s\" Value=\"%s\"/>\n";

static inline int MIB_GET(int id, void *value, size_t size)
{
	if (save_from_configd)
		return mib_get_s(id, value, size);
	else
		return _mib_get(id, value);
}

static inline unsigned int MIB_CHAIN_TOTAL(int id)
{
	if (save_from_configd)
		return mib_chain_total(id);
	else
		return _mib_chain_total(id);
}

#define MIB_CHAIN_GET(id, recordNum, chainEntry)			\
do {									\
	if (save_from_configd) {					\
		if(mib_chain_get(id, recordNum, chainEntry) == 0)	\
			DPRINT("\n %s %d\n", __func__, __LINE__);	\
	} else {							\
		chainEntry = _mib_chain_get(id, recordNum);		\
	}								\
} while (0)

static void print_name_value(FILE *fp, char *format_str, char *name, void *addr, TYPE_T type, int size, int depth, int encrypt)
{
	char string[MAX_MIB_VAL_STR_LEN];
	char tmp[MAX_MIB_VAL_STR_LEN];

	if(addr == NULL){
		DPRINT("%s %d\n", __FUNCTION__, __LINE__);
		return;
	}

	memset(string, 0, sizeof(string));
	memset(tmp, 0, sizeof(tmp));
	mib_to_string(string, addr, type, size);
#ifdef CONFIG_USER_XML_STRING_ENCRYPTION
	if(encrypt==1 && type == ENCRYPT_STRING_T){
		strcpy(tmp, string);
		rtk_xmlfile_str_encrypt(tmp, string);
	}
#endif
	TAB_DEPTH(fp, depth);
	bc += snprintf(strbuf+bc, OUT_STR_BUF_LEN-bc, format_str, name, string);
	DEBUGP(format_str, name, string);
}

static void print_chain_obj(FILE *fp, char *format_str, mib_chain_member_entry_T * desc, void *addr, int depth, int encrypt)
{
	mib_chain_member_entry_T * obj_desc;
	unsigned int entryNum = 0;
	int i, k, unit_size;
	void *pObj;

	obj_desc = desc->record_desc;
	if (obj_desc == NULL)
		return;

	unit_size = 0;
	for (i = 0; obj_desc[i].name[0]; i++) {
		unit_size += obj_desc[i].size;
	}

	if(unit_size != 0)
		entryNum = desc->size / unit_size;

	for (i = 0; i < entryNum; i++) {
		TAB_DEPTH(fp, depth);
		bc += snprintf(strbuf+bc, OUT_STR_BUF_LEN-bc, "<chain chainName=\"%s\"> <!--index=%d-->\n", desc->name, i);
		DEBUGP("<chain chainName=\"%s\"> <!--index=%d-->\n", desc->name, i);

		pObj = addr + unit_size * i;
		for (k = 0; obj_desc[k].name[0]; k++) {
			print_chain_member(fp, format_str, &obj_desc[k], pObj + obj_desc[k].offset, depth + 1, encrypt);
		}

		TAB_DEPTH(fp, depth);
		bc += snprintf(strbuf+bc, OUT_STR_BUF_LEN-bc, "</chain>\n");
		DEBUGP("</chain>\n");
	}
}

void print_chain_member(FILE *fp, char *format_str, mib_chain_member_entry_T * desc, void *addr, int depth, int encrypt)
{
	if(addr == NULL)
		return;

	switch (desc->type) {
	case OBJECT_T:
		print_chain_obj(fp, format_str, desc, addr, depth, encrypt);
		/* flush buffer to avoid potential buffer overflow */
		if (fp != stdout)
			xml_write(fp, &xor_idx);
		break;
	default:
		print_name_value(fp, format_str, desc->name, addr, desc->type, desc->size, depth, encrypt);
		break;
	}
	if (fp == stdout) {
		strbuf[bc] = '\0';
		fprintf(fp, "%s", strbuf);
		bc = 0;
	}
}

//return value==0, we will not skip
//return value==1, we will skip mib to config file
int check_mib_skip(const char *savefile, char* mibname)
{
	int ret = 0;
#ifdef CONFIG_USER_CONF_ON_XMLFILE
	FILE *fp = NULL;
        char *pstr=NULL;
	if(strcmp(savefile, CONF_ON_XMLFILE_HS)==0)
		return 0;

	if(strcmp(savefile, CONF_ON_XMLFILE)==0)
		return 0;
	
	fp = fopen(CONF_ON_XMLFILE_SKIP_EXPORT, "r");
	if(fp==NULL)
		return 0;

	while (!feof(fp)) {
		pstr = get_line(fp, LINE, sizeof(LINE));
		if(pstr !=NULL){
			if(strcmp(pstr, mibname)==0){
				ret = 1;
				break;
			}
		}
	}
	
	fclose(fp);
#endif
	return ret;
}

void do_mib_save_dependence(int mibid, void *addr, TYPE_T type, int size)
{
	char buf[64] = {0};
	int int_value = 0;

	switch (mibid){
#if defined(CONFIG_RTK_BOARDID)
		case MIB_HW_BOARD_ID:
			int_value = *(int *)addr;
			sprintf(buf,"%u",int_value);
			//printf("%s, set bootargs_boardid %s\n", __FUNCTION__, buf);
			rtk_env_set("bootargs_boardid", buf);
			break;
#endif
	}

	return;
}

static void xml_write(FILE *fp, int *xor_idx)
{
	if (bc) {
		if (bc >= OUT_STR_BUF_LEN) {
			DPRINT("Warning: string buffer full!\n");
			bc = OUT_STR_BUF_LEN;
		}
#ifdef XOR_ENCRYPT
		xor_encrypt2(strbuf, bc, xor_idx);
#endif
		strbuf[bc] = '\0';
		fwrite(strbuf, sizeof(char), bc, fp);
		bc = 0;
	}
}

#define _CONFIG_LOCK "/tmp/_config_xml_lock"
#define HS_TEMP_FILE "/var/_hs_config_tmp.xml"
#define CS_TEMP_FILE "/var/_config_tmp.xml"

int _save_xml_file(const char *savefile, CONFIG_DATA_T cnf_type, unsigned char flag, CONFIG_MIB_T range)
{
	int i, j, k;
	mib_chain_member_entry_T *rec_desc;
	unsigned int entryNum;
	unsigned char *chainEntry;
	void *buffer;
	FILE *fp = NULL;
	int fd;
	FILE *_fp = NULL;
	int _fd;
	FILE *lock_fp = NULL;
	int lock_fd;
	char *tmp_filename;
	char *lock_filename;
	int savefile_empty = 0;
	char mv_cmd[128];
#ifdef XOR_ENCRYPT
	char tempFile[32]={0};
	char cmd[64];
#endif
	xor_idx = 0;
	if (cnf_type == HW_SETTING) {
		tmp_filename = HS_TEMP_FILE;
	} else if (cnf_type == CURRENT_SETTING){
		tmp_filename = CS_TEMP_FILE;
	} else{
		return 0;
	}
	lock_filename = _CONFIG_LOCK;

	save_from_configd = flag;

	lock_fp = fopen(lock_filename, "w");
	if (lock_fp == NULL){
		DPRINT("Can't open lock file %s\n", lock_filename);
		return error;
	}
	lock_fd = fileno(lock_fp);
	flock(lock_fd, LOCK_EX);


	if( access( savefile, F_OK ) == 0 ) {
		_fp = fopen(savefile, "r");
	} else {
		_fp = fopen(savefile, "w");
		savefile_empty = 1;
	}
	if (_fp == NULL){
		fclose(lock_fp);
		return error;
	}

	_fd = fileno(_fp);
	flock(_fd, LOCK_EX);

	fp = fopen(tmp_filename, "w");
	if (fp == NULL){
		fclose(_fp);
		fclose(lock_fp);
		return error;
	}

	fd = fileno(fp);

	// init string buffer
	bc = 0;
	strbuf[bc] = '\0';

	if (cnf_type == HW_SETTING) {
		bc += snprintf(strbuf+bc, OUT_STR_BUF_LEN-bc, "%s\n", CONFIG_HEADER_HS);
		DEBUGP("%s\n", CONFIG_HEADER_HS);
		if(range != CONFIG_MIB_ALL){
			DPRINT("[%s] HW_SETTING NOT support, partial save config now.\n", __FUNCTION__);
			fclose(fp);
			fclose(_fp);
			fclose(lock_fp);
			return 0;
		}
	} else if (cnf_type == CURRENT_SETTING){
		bc += snprintf(strbuf+bc, OUT_STR_BUF_LEN-bc, "%s\n", CONFIG_HEADER);
		DEBUGP("%s\n", CONFIG_HEADER);

		/* step 1: backup and flash default datamodel */
		if(range != CONFIG_MIB_ALL){
			mib_backup((range == CONFIG_MIB_TABLE)?CONFIG_MIB_CHAIN:CONFIG_MIB_TABLE);
			mib_load(cnf_type, ((range == CONFIG_MIB_TABLE)?CONFIG_MIB_CHAIN:CONFIG_MIB_TABLE));
		}
	}else{
		fclose(fp);
		fclose(_fp);
		fclose(lock_fp);
		return 0;
	}

	// MIB Table
	buffer = NULL;
	for (i = 0; mib_table[i].id; i++) {
		if (mib_table[i].mib_type != cnf_type)
			continue;

		buffer = realloc(buffer, mib_table[i].size);
		if (buffer == NULL) {
			goto ERROR;
		}
		MIB_GET(mib_table[i].id, buffer, mib_table[i].size);
		if(check_mib_skip(savefile, mib_table[i].name)==0 && buffer != NULL){
			do_mib_save_dependence(mib_table[i].id, buffer, mib_table[i].type, mib_table[i].size);
			print_name_value(fp, FORMAT_STR, mib_table[i].name, buffer, mib_table[i].type, mib_table[i].size, 0, 1);
		}
		xml_write(fp, &xor_idx);
	}
	if(buffer) free(buffer);


	// MIB chain record
	for (i = 0; mib_chain_record_table[i].id; i++) {
		if (mib_chain_record_table[i].mib_type != cnf_type)
			continue;

		rec_desc = mib_chain_record_table[i].record_desc;
		if (rec_desc == NULL)
			entryNum = 0;
		else
			entryNum = MIB_CHAIN_TOTAL(mib_chain_record_table[i].id);

		DEBUGP("chain entry %d # %u\n", mib_chain_record_table[i].id, entryNum);

		if (entryNum == 0) {
			bc += snprintf(strbuf+bc, OUT_STR_BUF_LEN-bc, "<chain chainName=\"%s\">\n", mib_chain_record_table[i].name);
			DEBUGP("<chain chainName=\"%s\">\n", mib_chain_record_table[i].name);
			bc += snprintf(strbuf+bc, OUT_STR_BUF_LEN-bc, "</chain>\n");
			DEBUGP("</chain>\n");
		} else {
			if (save_from_configd) {
				chainEntry = malloc(mib_chain_record_table[i].per_record_size);
				if (chainEntry == NULL) {
					goto ERROR;
				}
			}

			for (j = 0; j < entryNum; j++) {
#ifdef CONFIG_00R0
				if(check_mib_skip(savefile, mib_chain_record_table[i].name)==0){
#endif
					bc += snprintf(strbuf+bc, OUT_STR_BUF_LEN-bc, "<chain chainName=\"%s\"> <!--index=%d-->\n", mib_chain_record_table[i].name, j);
					DEBUGP("<chain chainName=\"%s\"> <!--index=%d-->\n", mib_chain_record_table[i].name, j);
#ifdef CONFIG_00R0
				}
#endif

				MIB_CHAIN_GET(mib_chain_record_table[i].id, j, chainEntry);
				if(check_mib_skip(savefile, mib_chain_record_table[i].name)==0){
					for (k = 0; rec_desc[k].name[0]; k++) {
						print_chain_member(fp, FORMAT_STR, &rec_desc[k], chainEntry + rec_desc[k].offset, 1, 1);
						xml_write(fp, &xor_idx);
					}
				}

#ifdef CONFIG_00R0
				if(check_mib_skip(savefile, mib_chain_record_table[i].name)==0){
#endif
					bc += snprintf(strbuf+bc, OUT_STR_BUF_LEN-bc, "</chain>\n");
					DEBUGP("</chain>\n");
#ifdef CONFIG_00R0
				}
#endif
			}

			if (save_from_configd)
				free(chainEntry);
		}

	}

	if (cnf_type == HW_SETTING) {
		bc += snprintf(strbuf+bc, OUT_STR_BUF_LEN-bc, "%s\n", CONFIG_TRAILER_HS);
		DEBUGP("%s\n", CONFIG_TRAILER_HS);
	} else {
		bc += snprintf(strbuf+bc, OUT_STR_BUF_LEN-bc, "%s\n", CONFIG_TRAILER);
		DEBUGP("%s\n", CONFIG_TRAILER);
	}


	xml_write(fp, &xor_idx);

	/* Flush output buffer. */
	if (fflush(fp) != 0)
		DPRINT("fflush() error");
	/* Transfer data to storage. */
	if (fsync(fd) != 0)
		DPRINT("fsync() error");

	flock(_fd, LOCK_UN);
	fclose(_fp);

	fclose(fp);

#ifdef CONFIG_USER_XML_BACKUP
	if (!savefile_empty) {
		generate_xmlfile_backup2(cnf_type);
	}
#endif

	snprintf(mv_cmd, 128, "mv -f %s %s", tmp_filename, savefile);
	system(mv_cmd);

	system("sync");

	flock(lock_fd, LOCK_UN);
	fclose(lock_fp);



#ifdef CONFIG_USER_FLATFSD_XXX
	system("flatfsd -s");
#endif

#if 1 //for check config partition write count
#define CONFIG_PARTITION_PATH "/var/run/.config_partition_write_count"
	int cs_count = 0, hs_count = 0;
	long cs_time = 0, hs_time = 0;
	struct sysinfo info;
	
	fp = fopen(CONFIG_PARTITION_PATH, "r+");
	if(fp != NULL) {
		if(fscanf(fp, "CS: %d , UpdateTime: %ld\n", &cs_count, &cs_time) != 2)
			DPRINT("fscanf failed: %s %d\n", __func__, __LINE__);
		if(fscanf(fp, "HS: %d , UpdateTime: %ld\n", &hs_count, &hs_time) != 2)
			DPRINT("fscanf failed: %s %d\n", __func__, __LINE__);
	}
	else fp = fopen(CONFIG_PARTITION_PATH, "w");
	
	if(fp)
	{
		sysinfo(&info);
		if(cnf_type == HW_SETTING){ hs_count++; hs_time = info.uptime; }
		else{ cs_count++; cs_time = info.uptime;}
		fseek(fp, 0, SEEK_SET); 
		fprintf(fp, "CS: %-8d , UpdateTime: %ld\n", cs_count, cs_time);
		fprintf(fp, "HS: %-8d , UpdateTime: %ld\n", hs_count, hs_time);
		fclose(fp);
	}
#endif

	/* step 2: restore datamodel */
	if (cnf_type == CURRENT_SETTING){
		if(range != CONFIG_MIB_ALL){
			mib_restore(((range == CONFIG_MIB_TABLE)?CONFIG_MIB_CHAIN:CONFIG_MIB_TABLE));
		}
	}

	return 0;
ERROR:
	if (cnf_type == CURRENT_SETTING){
		if(range != CONFIG_MIB_ALL){
			mib_restore(((range == CONFIG_MIB_TABLE)?CONFIG_MIB_CHAIN:CONFIG_MIB_TABLE));
		}
	}
	if (NULL != fp) {
		fclose(fp);
		fp = NULL;
	}
	if(NULL != _fp){
		fclose(_fp);
		_fp = NULL;
	}
	if(NULL != lock_fp){
		fclose(lock_fp);
		lock_fp = NULL;
	}

	return error;
}

