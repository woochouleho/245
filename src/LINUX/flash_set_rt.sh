#!/bin/ash
#
# usage: flash.sh [cmd] ...
#

#set -x 
DEFAULT_FILE="/etc/config_default.xml"
DEFAULT_HS_FILE="/etc/config_default_hs.xml"
BACKUP_SUFFIX="bak"

#cs
DEFAULT_RT_FILE="/var/config/config_default_rt.xml"
CONFIG_DEFAULT_RT_FILE="/etc/config_default_rt.xml"
CONFIG_RT_XML_NAME="config_default_rt.xml"
DEFAULT_RT_FILE_TMP="/var/config/config_default_rt_tmp.xml"
#hs
DEFAULT_HS_RT_FILE="/var/config/config_default_hs_rt.xml"
CONFIG_DEFAULT_HS_RT_FILE="/etc/config_default_hs_rt.xml"
CONFIG_RT_HS_XML_NAME="config_default_hs_rt.xml"
#
tmpfile="/var/config/tmpfile.txt"
tmpLineNum="/var/config/tmpNum.txt"
dirFlagfile="/var/config/dirFlagfile.txt"
dirName=""
xmlMibName=""
#---end


show_usage() {
	echo 'Usage: flash_set_rt.sh [cmd]'
	echo 'cmd:'
	
if [ -f $DEFAULT_RT_FILE ];then
	echo '  -T <cs> [ENTRY-NAME] [MIB-NAME] [MIB-VALUE] [ENTRY-INDEX]   : set a specific default mib value on dut runtime.'
	echo "		[ENTRY_NAME]: If you want to set the value of a single MIB_TABLE, please keep it as 'MIB_TABLE' If you want to set the members of Chain, please enter the corresponding Chain Name."
	echo "		[ENTRY_INDEX]: If there is no such value, then do not write. When setting for the first time, add the index value from 0" 
fi
}

rm_file() {
	[ -f ${1} ] && rm ${1}
}

#---
get_noChainCmpMibDir() {
chainNameStr="<chain chainName="
i=1
cat $1 | while read line
do
	noChainMibName_tmp=`echo "${line}" | grep "$xmlMibName"`

	if [  "$noChainMibName_tmp" ];then
		echo ${line} > $tmpfile
		echo $i > $tmpLineNum
	fi

	chainStrFlag=`echo "${line}" | grep "$chainNameStr"`

	if [ "$chainStrFlag" ];then
		break
	fi

	i=$(( $i+1 ))
done
}

get_cmpMibDir() {
dirName_tt=`echo ${dirName/%\"/\\\"}`
cmpMibDirFlag=0
i=1
cat $1 | while read line
do
	dirName_tmp=`echo "${line}" | grep "$dirName_tt"`

	if [ "$dirName_tmp" == "$dirName" ];then
		cmpMibDirFlag=1
		echo 1 > $dirFlagfile
	fi
	
	if [ $cmpMibDirFlag -eq 1 ];then
		xmlMibName_tmp=`echo "${line}" | grep "$xmlMibName"`
		
		if [  "$xmlMibName_tmp" ];then
			echo ${line} > $tmpfile
			echo $i > $tmpLineNum
		fi
		
		if [ "${line}" == "</chain>" ];then
			cmpMibDirFlag=0
			break
		fi
        fi
	i=$(( $i+1 ))
done
}

set_default_rt_value() {
#echo "---------set default rt value----------"
dirFlag=0
entryName=$2
mibName=$3
mibValue=$4
cmpMibStr_num=0
#noChain
noChain_flag=0
configEndLine=""
configStartLine=""
noChaincmpMibStr=""
noChainMibStr_num=0

if [ $entryName == "MIB_TABLE" ];then
	noChain_flag=1;
fi

if [ $# -eq 5 ];then
#	dirName="<Dir Name=\"$entryName\"> <!--index=$5-->"
	dirName="<chain chainName=\"$entryName\"> <!--index=$5-->"
elif [ $# -eq 4 ];then
#       dirName="<Dir Name=\"$entryName\">"
	dirName="<chain chainName=\"$entryName\">"
else
	echo "The cmd format error"
	return 0
fi

if [ "$1" == "cs" ];then
	SET_FILE=$DEFAULT_RT_FILE
	SET_XML_NAME=$CONFIG_RT_XML_NAME
	configEndLine="<\/Config_Information_File_8671>"
	configStartLine="<Config_Information_File_8671>"
elif [ "$1" == "hs" ];then
	SET_FILE=$DEFAULT_HS_RT_FILE
        SET_XML_NAME=$CONFIG_RT_HS_XML_NAME
	configEndLine="<\/Config_Information_File_HS>"
	configStartLine="<Config_Information_File_HS>"
fi

#MIB process
xmlMibName="<Value Name=\"$mibName\" Value="
xmlMibValue="<Value Name=\"$mibName\" Value=\"$mibValue\"/>"
xmlMibValue_t="<Value Name=\"$mibName\" Value=\"$mibValue\"\/>"

if [ $noChain_flag -eq 0 ];then
	get_cmpMibDir $SET_FILE

	if [ -f $dirFlagfile ];then
		dirFlag=`cat $dirFlagfile`
		rm -f $dirFlagfile
	fi

	if [ $dirFlag -eq 1 ];then
		if [ -f $tmpfile ];then
			cmpMibStr=`cat $tmpfile`
			rm -f $tmpfile
		fi
		if [ -f $tmpLineNum ];then
			cmpMibStr_num=`cat $tmpLineNum`
			rm -f $tmpLineNum
		fi
	fi
else #noChain
	dirFlag=0

	get_noChainCmpMibDir $SET_FILE
	if [ -f $tmpfile ];then
		noChaincmpMibStr=`cat $tmpfile`
		rm -f $tmpfile
	fi

	if [ -f $tmpLineNum ];then
		noChainMibStr_num=`cat $tmpLineNum`
		rm -f $tmpLineNum
	fi
fi

if [ $noChain_flag -eq 0 ];then
	if [ $dirFlag -eq 1 ];then
		if [ "$cmpMibStr" ];then
#			cmpMibStr_t=`echo ${cmpMibStr/%\/>/\\\/>}`
			cd /var/config
			sed -i "${cmpMibStr_num}c  $xmlMibValue_t" ab $SET_XML_NAME
			cd - > /dev/null
#		       	sed -i "/$dirName/{:a;n;s/$cmpMibStr_t/$xmlMibValue_t/g;/<\/Dir>/!ba}" $SET_FILE
	    	else
        		sed -i "/$dirName/a\\  $xmlMibValue"  $SET_FILE   #insert a new mib below the table name 
	    	fi
	else
#       	sed -i "/<\/Config>/i\ $dirName\n  $xmlMibValue\n </Dir>"  $SET_FILE
		sed -i "/${configEndLine}/i\ $dirName"  $SET_FILE
		sed -i "/${configEndLine}/i\  $xmlMibValue"  $SET_FILE
		sed -i "/${configEndLine}/i\ </chain>"  $SET_FILE
	fi
else #noChain_flag=1
	if [ "$noChaincmpMibStr" ];then   #mib already exist
		cd /var/config
		sed -i "${noChainMibStr_num}c  $xmlMibValue_t" ab $SET_XML_NAME
		cd - > /dev/null
	else
#		sed -i "/${configEndLine}/i\  $xmlMibValue"  $SET_FILE #inset the before last line
		sed -i "/${configStartLine}/a\\ $xmlMibValue"  $SET_FILE   #insert a new mib below the <config header> second line
	fi
fi
}

case "$1" in
  "-T")
	if [ "$2" = "cs" ]; then
		if [ ! -f $CONFIG_DEFAULT_RT_FILE ];then
			[ -f $DEFAULT_RT_FILE ] && rm_file $DEFAULT_RT_FILE
			echo "---Don't support [$1 $2] cmd---"
			show_usage
                	exit 0
		fi
	
		echo "------ [$1] Set a specific mib parameter into [$DEFAULT_RT_FILE]. ------"
	
		if [[ $# -ne 5 && $# -ne 6 ]];then                
			echo "---Cmd format error---"
			show_usage
			exit 0
        	fi
        
		[ ! -f $DEFAULT_RT_FILE ]&& cp -f $CONFIG_DEFAULT_RT_FILE $DEFAULT_RT_FILE
	
		[ -f ${DEFAULT_RT_FILE} ] && cp -f $DEFAULT_RT_FILE $DEFAULT_RT_FILE_TMP
		
		if [ $# -eq 6 ];then
		        set_default_rt_value cs $3 $4 $5 $6
		else
			set_default_rt_value cs $3 $4 $5
		fi
		
		[ -f ${DEFAULT_RT_FILE} ] && /bin/flash xmlchk $DEFAULT_RT_FILE
		if [ "$?" = "0" ]; then
			rm_file $DEFAULT_RT_FILE_TMP
		else
			echo "the [$DEFAULT_RT_FILE] format is incorect,please re-set"
			cp $DEFAULT_RT_FILE_TMP $DEFAULT_RT_FILE
			rm_file $DEFAULT_RT_FILE_TMP
		fi
		echo "Please flash default cs  system."
	elif [ "$2" = "hs" ]; then
###don't support set_rt for hw setting
		show_usage
		exit 0
###
                if [ ! -f $CONFIG_DEFAULT_HS_RT_FILE ];then
                        [ -f $DEFAULT_HS_RT_FILE ] && rm_file $DEFAULT_HS_RT_FILE
                        echo "---Don't support [$1 $2] cmd---"
                       	show_usage
                        exit 0
                fi

                echo "------ [$1] Set a specific mib parameter into [$DEFAULT_HS_RT_FILE]. ------"

                if [[ $# -ne 5 && $# -ne 6 ]];then
                        echo "---Cmd format error---"
                        show_usage
                        exit 0
                fi

                [ ! -f $DEFAULT_HS_RT_FILE ]&& cp -f $CONFIG_DEFAULT_HS_RT_FILE $DEFAULT_HS_RT_FILE

                [ -f ${DEFAULT_HS_RT_FILE} ] && cp -f $DEFAULT_HS_RT_FILE $DEFAULT_RT_FILE_TMP

                if [ $# -eq 6 ];then
                        set_default_rt_value hs $3 $4 $5 $6
                else
                        set_default_rt_value hs $3 $4 $5
                fi

                [ -f ${DEFAULT_HS_RT_FILE} ] && /bin/flash xmlchk  $DEFAULT_HS_RT_FILE
                if [ "$?" = "0" ]; then
                        rm_file $DEFAULT_RT_FILE_TMP
                else
                        echo "the [$DEFAULT_HS_RT_FILE] format is incorect,please re-set"
                        cp $DEFAULT_RT_FILE_TMP $DEFAULT_HS_RT_FILE
                        rm_file $DEFAULT_RT_FILE_TMP
                fi
		echo "Please flash default hs  system."
	fi
#	echo "Please flash default $2  system."
	exit 0
        ;;
  *)
  	show_usage
	exit 1
	;;
esac
