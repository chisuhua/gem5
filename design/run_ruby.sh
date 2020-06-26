#!/bin/bash
# -*- coding: utf-8 -*-


#TEST=configs/learning_gem5/part3/simple_ruby.py
TEST=configs/learning_gem5/part3/ruby_test.py

OPTION="--debug-flags=ProtocolTrace,RubyTest,RubyQueue,RubyMessageBuffer,RubySequencer"
#OPTION="--debug-flags=ProtocolTrace,RubyTest,RubyQueue,RubyMessageBuffer,RubySequencer"
#TEST=configs/learning_gem5/part3/simple_ruby.py

RUN_GEM5=./build/X86_MSI/gem5.debug
#RUN_GEM5=./build/X86_VI_hammer/gem5.debug
#RUN_GEM5=./build/NULL_MOESI_hammer/gem5.debug

echo $RUN_GEM5 $OPTION $TEST
$RUN_GEM5 $OPTION $TEST
