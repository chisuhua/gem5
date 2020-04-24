/*
 * Copyright (c) 2006 The Regents of The University of Michigan
 * Copyright (c) 2017 The University of Virginia
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Gabe Black
 *          Ali Saidi
 *          Alec Roelke
 */

#ifndef __PPU_PROCESS_HH__
#define __PPU_PROCESS_HH__

#include <string>
#include <vector>

#include "mem/page_table.hh"
#include "ppu_sim/process.hh"

class ObjectFile;
class PpuSOCSystem;

class PpuProcess : public PpuSOCProcess
{
  protected:
    PpuProcess(PpuSOCProcessParams * params, ObjectFile *objFile);
    template<class IntType>
    void argsInit(int pageSize);

  public:
    RegVal getSyscallArg(ThreadContext *tc, int &i) override;
    /// Explicitly import the otherwise hidden getSyscallArg
    using PpuSOCProcess::getSyscallArg;
    void setSyscallReturn(ThreadContext *tc,
                          SyscallReturn return_value) override;

    virtual bool mmapGrowsDown() const override { return false; }
};

class PpuProcess64 : public PpuProcess
{
  protected:
    PpuProcess64(PpuSOCProcessParams * params, ObjectFile *objFile);
    void initState() override;
};

class PpuProcess32 : public PpuProcess
{
  protected:
    PpuProcess32(PpuSOCProcessParams * params, ObjectFile *objFile);
    void initState() override;
};

#endif // __PPU_PROCESS_HH__
