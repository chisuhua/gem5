/*
 * Copyright (c) 2017 The University of Virginia
 * Copyright 2015 LabWare
 * Copyright 2014 Google, Inc.
 * Copyright (c) 2007 The Regents of The University of Michigan
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
 * Authors: Nathan Binkert
 *          Boris Shingarov
 *          Alec Roelke
 */

#ifndef __ARCH_PPU_REMOTE_GDB_HH__
#define __ARCH_PPU_REMOTE_GDB_HH__

#include <string>

#include "arch/ppu/registers.hh"
#include "ppu_base/remote_gdb.hh"

class PpuSOCSystem;
class ThreadContext;

namespace PpuISA
{

class RemoteGDB : public PpuBaseRemoteGDB
{
  protected:
    static const int ExplicitCSRs = 4;
    static const int NumGDBRegs = 4162;
    static const int NumCSRs = 4096;

    bool acc(Addr addr, size_t len) override;
    // A breakpoint will be 2 bytes if it is compressed and 4 if not
    bool checkBpLen(size_t len) override { return len == 2 || len == 4; }

    class PpuGdbRegCache : public PpuBaseGdbRegCache
    {
      using PpuBaseGdbRegCache::PpuBaseGdbRegCache;
      private:
        struct {
            uint64_t gpr[NumIntArchRegs];
            uint64_t pc;
            FloatReg fpr[NumFloatRegs];

            // MiscReg csr_base;
            uint32_t fflags;
            uint32_t frm;
            uint32_t fcsr;
            // MiscReg csr[NumMiscRegs - ExplicitCSRs -1];
        } __attribute__((__packed__)) r;

       public:
        char *data() const { return (char *)&r; }
        size_t size() const { return sizeof(r); }
        void getRegs(ThreadContext*);
        void setRegs(ThreadContext*) const;

        const std::string
        name() const
        {
            return gdb->name() + ".PpuGdbRegCache";
        }
    };

    /**
     * 32 Bit architecture
     */
    class Ppu32GdbRegCache : public PpuBaseGdbRegCache
    {
      using PpuBaseGdbRegCache::PpuBaseGdbRegCache;
      private:
        struct {
            uint32_t gpr[NumIntArchRegs];
            uint32_t pc;
            // uint32_t fpr[NumFloatRegs];
            // uint32_t dcsr;
            // uint32_t dpc;
            // uint32_t dscratch;
        } __attribute__((__packed__)) r;
      public:
        char *data() const { return (char *)&r; }
        size_t size() const { return sizeof(r); }
        void getRegs(ThreadContext*);
        void setRegs(ThreadContext*) const;

        const std::string
        name() const
        {
            return gdb->name() + ".Ppu32GdbRegCache";
        }
    };

    PpuGdbRegCache regCache;
    Ppu32GdbRegCache regCache32;

  public:
    RemoteGDB(PpuSOCSystem *_system, ThreadContext *tc, int _port);
    PpuBaseGdbRegCache *gdbRegs() override;
};

} // namespace PpuISA

#endif /* __ARCH_PPU_REMOTE_GDB_H__ */