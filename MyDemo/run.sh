#!/bin/bash

ROOT=..

pushd ${ROOT}

LOG_FILE="./myDemoLog.txt"
rm -rf ${LOG_FILE}
touch ${LOG_FILE}
gnome-terminal --command="tail -f ${LOG_FILE}"

./Robot_gui ./MyDemo/build/main -m ./maps/1.txt 

popd
