#!/bin/sh

###ensure the cfg_province was set in
PROVINCEPATH=/etc/province
PROVINCEFILE=
ENV_NAME=cfg_province
RESET_ENV_NAME=rst2dfl
PROVINCE=`nv getenv $ENV_NAME`
RESET_FLAG=`nv getenv $RESET_ENV_NAME`

if [ -z "${PROVINCE}" ];then
	echo "[CONFIG] do not deal this situation!"
else
	PROVINCE=${PROVINCE/*=/}
	####check file exist or not
	PROVINCEFILE=$PROVINCEPATH/config_${PROVINCE}.xml
	if [ ! -f $PROVINCEFILE ];then
		echo "[CONFIG][ERROR] Not found province file (${PROVINCE}) !!!"
		####clear the cfg_province
		nv setenv $ENV_NAME
		exit 0
	else
		####clear the cfg_province
		nv setenv $ENV_NAME 
		echo "[CONFIG] ========> Update province (${PROVINCE}) <========"
		####set the value of HW_PROVINCE_NAME
		flash set HW_PROVINCE_NAME ${PROVINCE} &> /dev/null
		if [ $? -eq 0 ]; then
			echo "[CONFIG] ========> !!! Do factory reset !!! <========"
			flash default cs &> /dev/null
			if [ ! $? -eq 0 ]; then
				echo "[CONFIG][ERROR] Fail to do factory reset !"
			fi
		else
			echo "[CONFIG][ERROR] Fail to update province (${PROVINCE}) !"
		fi
	fi
fi

if [ -z "${RESET_FLAG}" ];then
    echo "[CONFIG] no ${RESET_ENV_NAME} found"
else
	RESET_FLAG=${RESET_FLAG/*=/}
	nv setenv $RESET_ENV_NAME
	if [ "${RESET_FLAG}" == "1" ]; then
		echo "[CONFIG] ========> !!! RESET_FLAG=1, Do factory reset !!! <========"
		flash default cs &> /dev/null
		if [ ! $? -eq 0 ]; then
			echo "[CONFIG][ERROR] Fail to do factory reset !"
		fi
	fi
fi

exit 0
