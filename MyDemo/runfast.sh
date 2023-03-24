#!/bin/bash

ROOT=..
MAP=./pre_official_map

MAP_NO=$1
if [ -z ${MAP_NO} ]; then
	echo "usage ${0} <map>"
	exit
fi

pushd ${ROOT}


LOG_ON=`cat MyDemo/src/config.h | grep LOG_ON | cut -d '(' -f2 | cut -d ')' -f1`

if [ ${LOG_ON} -ne 0 ]; then
	LOG_FILE="./myDemoLog.txt"
	NO=`ps aux | grep "tail -f ${LOG_FILE}" | wc -l`
	if [ ${NO} -ne 2 ]; then
		rm -rf ${LOG_FILE}
		touch ${LOG_FILE}
		gnome-terminal --command="tail -f ${LOG_FILE}" --hide-menubar
	fi
fi

./Robot ./MyDemo/build/main -m ${MAP}/${MAP_NO}.txt -f

popd
