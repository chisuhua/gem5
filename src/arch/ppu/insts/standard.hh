/*
 * Copyright (c) 2015 RISC-V Foundation
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
 * Authors: Alec Roelke
 */

#ifndef __ARCH_PPU_STANDARD_INST_HH__
#define __ARCH_PPU_STANDARD_INST_HH__

#include <string>

#include "arch/ppu/insts/bitfields.hh"
#include "arch/ppu/insts/static_inst.hh"
#include "ppu/exec_context.hh"
#include "ppu/static_inst.hh"

namespace PpuISA
{

/**
 * Base class for operations that work only on registers
 */
class RegOp : public PpuStaticInst
{
  protected:
    using PpuStaticInst::PpuStaticInst;

    std::string generateDisassembly(
        Addr pc, const SymbolTable *symtab) const override;
};

/**
 * Base class for operations with immediates (I is the type of immediate)
 */
template<typename I>
class ImmOp : public PpuStaticInst
{
  protected:
    I imm;

    ImmOp(const char *mnem, MachInst _machInst, OpClass __opClass)
        : PpuStaticInst(mnem, _machInst, __opClass), imm(0)
    {}
};

/**
 * Base class for system operations
 */
class SystemOp : public PpuStaticInst
{
  protected:
    using PpuStaticInst::PpuStaticInst;

    std::string
    generateDisassembly(Addr pc, const SymbolTable *symtab) const override
    {
        return mnemonic;
    }
};

/**
 * Base class for CSR operations
 */
class CSROp : public PpuStaticInst
{
  protected:
    uint64_t csr;
    uint64_t uimm;

    /// Constructor
    CSROp(const char *mnem, MachInst _machInst, OpClass __opClass)
        : PpuStaticInst(mnem, _machInst, __opClass),
            csr(FUNCT12), uimm(CSRIMM)
    {}

    std::string generateDisassembly(
        Addr pc, const SymbolTable *symtab) const override;
};

}

#endif // __ARCH_PPU_STANDARD_INST_HH__