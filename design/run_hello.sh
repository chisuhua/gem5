#!/bin/bash
# -*- coding: utf-8 -*-

OPTION=""
RUN=./build/X86_VI_hammer/gem5.debug
#CONF="configs/example/simple_gpgpu.py"
CONF="configs/example/se.py"
CMD="-c ./design/gpgpu/gpgpu-sim/cuda_samples/0_Simple/hello/hello"
GDB="-w"

echo $RUN $OPTION $CONF $GDB $CMD
$RUN $OPTION $CONF $GDB $CMD
