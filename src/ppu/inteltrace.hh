/*
 * Copyright (c) 2001-2005 The Regents of The University of Michigan
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
 * Authors: Steve Reinhardt
 *          Nathan Binkert
 */

#ifndef __PPU_INTELTRACE_HH__
#define __PPU_INTELTRACE_HH__

#include "base/trace.hh"
#include "base/types.hh"
#include "ppu/static_inst.hh"
#include "ppu/thread_context.hh"
#include "debug/PpuExecEnable.hh"
#include "params/IntelTrace.hh"
#include "sim/insttracer.hh"

#ifdef BUILD_PPU
using namespace PpuISA;
#endif

namespace Trace {

class IntelTraceRecord : public InstRecord
{
  public:
    IntelTraceRecord(Tick _when, PpuThreadContext *_thread,
               const StaticInstPtr _staticInst, ThePpuISA::PCState _pc,
               const StaticInstPtr _macroStaticInst = NULL)
        : InstRecord(_when, _thread, _staticInst, _pc,
                _macroStaticInst)
    {
    }

    void dump();
};

class IntelTrace : public InstTracer
{
  public:

    IntelTrace(const IntelTraceParams *p) : InstTracer(p)
    {}

    IntelTraceRecord *
    getInstRecord(Tick when, PpuThreadContext *tc,
            const StaticInstPtr staticInst, ThePpuISA::PCState pc,
            const StaticInstPtr macroStaticInst = NULL)
    {
        if (!Debug::PpuExecEnable)
            return NULL;

        return new IntelTraceRecord(when, tc, staticInst, pc, macroStaticInst);
    }
};

} // namespace Trace

#endif // __PPU_INTELTRACE_HH__