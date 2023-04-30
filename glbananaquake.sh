#!/bin/bash

ARCH=$(arch)

echo "############################################################"
echo "################# STARTING GLBANANAQUAKE ###################"
echo "############################################################"


./WinQuake/debug${ARCH}/bin/glquake -basedir ./WinQuake -sw -w 1600 -h 900 +map e1m1
