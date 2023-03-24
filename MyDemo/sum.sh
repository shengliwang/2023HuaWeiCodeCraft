#!/bin/bash

ROOT=..

MAP=./pre_official_map

pushd ${ROOT} &> /dev/null


STR1=`./Robot ./MyDemo/build/main -m ${MAP}/1.txt -f 2>&1 | grep score`
echo $STR1
score1=`echo $STR1| cut -d ',' -f2 | cut -d ':' -f2 | cut -d '}' -f1`
STR2=`./Robot ./MyDemo/build/main -m ${MAP}/2.txt -f 2>&1 | grep score`
echo $STR2
score2=`echo $STR2| cut -d ',' -f2 | cut -d ':' -f2 | cut -d '}' -f1`
STR3=`./Robot ./MyDemo/build/main -m ${MAP}/3.txt -f 2>&1 | grep score`
echo $STR3
score3=`echo $STR3| cut -d ',' -f2 | cut -d ':' -f2 | cut -d '}' -f1`
STR4=`./Robot ./MyDemo/build/main -m ${MAP}/4.txt -f 2>&1 | grep score`
echo $STR4
score4=`echo $STR4| cut -d ',' -f2 | cut -d ':' -f2 | cut -d '}' -f1`

let TOTAL=score1+score2+score3+score4

echo "TOTAL:$score1+$score2+$score3+$score4=$TOTAL"


popd &> /dev/null
