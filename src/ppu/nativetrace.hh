/*
 * Copyright (c) 2006-2009 The Regents of The University of Michigan
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
 */

#ifndef __PPU_NATIVETRACE_HH__
#define __PPU_NATIVETRACE_HH__

#include <unistd.h>

#include <cerrno>

#include "base/socket.hh"
#include "base/trace.hh"
#include "base/types.hh"
#include "ppu/exetrace.hh"
#include "ppu/static_inst.hh"

#if 0
#ifdef BUILD_PPU
namespace PpuISA {
#endif

class PpuThreadContext;
#ifdef BUILD_PPU
};
using namespace PpuISA;
#endif
#endif
class PpuThreadContext;

namespace Trace {

class NativeTrace;

class NativeTraceRecord : public ExeTracerRecord
{
  protected:
    NativeTrace * parent;

  public:
    NativeTraceRecord(NativeTrace * _parent,
               Tick _when, PpuThreadContext *_thread,
               const StaticInstPtr _staticInst, ThePpuISA::PCState _pc,
               const StaticInstPtr _macroStaticInst = NULL)
        : ExeTracerRecord(_when, _thread, _staticInst, _pc, _macroStaticInst),
        parent(_parent)
    {
    }

    void dump();
};

class NativeTrace : public ExeTracer
{
  protected:
    int fd;

    ListenSocket native_listener;

  public:

    NativeTrace(const Params *p);
    virtual ~NativeTrace() {}

    NativeTraceRecord *
    getInstRecord(Tick when, PpuThreadContext *tc,
            const StaticInstPtr staticInst, ThePpuISA::PCState pc,
            const StaticInstPtr macroStaticInst = NULL)
    {
        return new NativeTraceRecord(this, when, tc,
                staticInst, pc, macroStaticInst);
    }

    template<class T>
    bool
    checkReg(const char * regName, T &val, T &realVal)
    {
        if (val != realVal)
        {
            DPRINTFN("Register %s should be %#x but is %#x.\n",
                    regName, realVal, val);
            return false;
        }
        return true;
    }

    void
    read(void *ptr, size_t size)
    {
        size_t soFar = 0;
        while (soFar < size) {
            ssize_t res = ::read(fd, (uint8_t *)ptr + soFar, size - soFar);
            if (res < 0)
                panic("Read call failed! %s\n", strerror(errno));
            else
                soFar += res;
        }
    }

    virtual void
    check(NativeTraceRecord *record) = 0;
};

} // namespace Trace

#endif // __PPU_NATIVETRACE_HH__