#!/bin/bash
# -*- coding: utf-8 -*-
#OPTION="-v -d=CxxConfig,Event"

# run ../../run_sc.sh first
# make sure use correct master/slave

#OPTION="-v -d CxxConfig -d TrafficGen -d Event -e 10000"
#OPTION="-v -d TrafficGen -d Event -e 2000000"
#OPTION="-v -e 500000 -d TrafficGen -d Event"
#OPTION="-v -e 500000 -d Event"
#OPTION="-v -e 5000000 -d TrafficGen"
#OPTION="-v -d TrafficGen"
OPTION="-v -d TrafficGen -d MemTest"

CMD="./build/examples/slave_port/gem5.sc m5out/config.ini $OPTION "
#CMD="./build/examples/master_port/gem5.sc m5out/config.ini $OPTION "
#echo $CMD
$CMD
