#!/bin/bash

ROOT=..

pushd ${ROOT}


LOG_ON=`cat MyDemo/src/config.h | grep LOG_ON | cut -d '(' -f2 | cut -d ')' -f1`

if [ ${LOG_ON} -ne 0 ]; then
	LOG_FILE="./myDemoLog.txt"
	rm -rf ${LOG_FILE}
	touch ${LOG_FILE}
	gnome-terminal --command="tail -f ${LOG_FILE}" --hide-menubar
fi

./Robot_gui ./MyDemo/build/main -m ./maps/1.txt 

popd
