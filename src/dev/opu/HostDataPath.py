#!/usr/bin/env python
# -*- coding: utf-8 -*-
# vim: ts=4 sw=4 sts=4 expandtab
from m5.params import *
from m5.SimObject import SimObject

class HostDataPath(SimObject):
    type = 'HostDataPath'
    cxx_header = "dev/opu/host_datapath.hh"
    cxx_class = 'gem5::HostDataPath'

    data_port = ResponsePort("CPU side port, receives requests")
    range = Param.AddrRange('4GB', "Address ragen")
