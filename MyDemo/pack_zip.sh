#!/bin/bash

SCRIPT=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT")
cd $BASEDIR/src

if [ ! -f CMakeLists.txt ]
then
    echo "ERROR: $BASEDIR is not a valid CMake file of SDK_C for CodeCraft-2023."
    echo "  Please run this script in a regular directory of SDK_C."
    exit -1
fi

rm -f ../CodeCraft-2023.zip

LOG_ON=`cat config.h | grep LOG_ON | cut -d '(' -f2 | cut -d ')' -f1`

if [ ${LOG_ON} -ne 0 ]; then
	echo -e "\033[31mLOG is enable, please disable!\033[00m"
	exit
fi



zip -9 -r CodeCraft-2023.zip *.cpp *.cc *.c *.hpp *.h CMakeLists.txt
mv  CodeCraft-2023.zip ../
