#!/bin/bash
# -*- coding: utf-8 -*-
#OPTION="-v -d=CxxConfig,Event"

# run ../../run_sc.sh first
# make sure use correct master/slave

OPTION="-v -d CxxConfig"

CMD="./build/examples/slave_port/gem5.sc m5out/config.ini $OPTION "
#CMD="./build/examples/master_port/gem5.sc m5out/config.ini $OPTION "
#echo $CMD
$CMD
