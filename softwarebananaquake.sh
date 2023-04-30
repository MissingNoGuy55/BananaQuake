#!/bin/bash

ARCH=$(arch)

echo "############################################################"
echo "################## STARTING BANANAQUAKE ####################"
echo "############################################################"


./WinQuake/debug${ARCH}/bin/softwarebananaquake -basedir ./WinQuake -sw -w 1600 -h 900 +map e1m1
