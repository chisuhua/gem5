/*
 * Copyright 2015 LabWare
 * Copyright 2014 Google, Inc.
 * Copyright (c) 2010 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2017 The University of Virginia
 * Copyright (c) 2002-2005 The Regents of The University of Michigan
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
 */

/*
 * Copyright (c) 1990, 1993 The Regents of the University of California
 * All rights reserved
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Lawrence Berkeley Laboratories.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)kgdb_stub.c 8.4 (Berkeley) 1/12/94
 */

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * $NetBSD: kgdb_stub.c,v 1.8 2001/07/07 22:58:00 wdk Exp $
 *
 * Taken from NetBSD
 *
 * "Stub" to allow remote cpu to debug over a serial line using gdb.
 */

#include "arch/ppu/remote_gdb.hh"

#include <string>

#include "arch/ppu/pagetable_walker.hh"
#include "arch/ppu/registers.hh"

// #include "arch/ppu/registers.hh"
// #include "arch/ppu/system.hh"
// #include "arch/ppu/utility.hh"
#include "arch/ppu/tlb.hh"
#include "debug/PpuGDBAcc.hh"
#include "mem/page_table.hh"
#include "ppu/thread_state.hh"
#include "sim/full_system.hh"

using namespace std;
namespace PpuISA
{

RemoteGDB::RemoteGDB(PpuSOCSystem *_system, ThreadContext *tc, int _port)
    : PpuBaseRemoteGDB(_system, tc, _port), regCache(this), regCache32(this)
{
}

bool
RemoteGDB::acc(Addr va, size_t len)
{
    if (PpuFullSystem)
    {
        TLB *tlb = dynamic_cast<TLB *>(context()->getDTBPtr());
        unsigned logBytes;
        Addr paddr = va;

        PrivilegeMode pmode = tlb->getMemPriv(context(), BaseTLB::Read);
        SATP satp = context()->readMiscReg(MISCREG_SATP);
        if (pmode != PrivilegeMode::PRV_M &&
            satp.mode != AddrXlateMode::BARE) {
            Walker *walker = tlb->getWalker();
            Fault fault = walker->startFunctional(
                    context(), paddr, logBytes, BaseTLB::Read);
            if (fault != NoFault)
                return false;
        }
        return true;
    }
    // return context()->PpugetProcessPtr()->pTable->lookup(va) != nullptr;
    return (dynamic_cast<PpuThreadContext*>(context()))->PpugetProcessPtr()->pTable->lookup(va) != nullptr;
}

void
RemoteGDB::PpuGdbRegCache::getRegs(ThreadContext *context_)
{
    PpuThreadContext* context = dynamic_cast<PpuThreadContext*>(context_);
    DPRINTF(PpuGDBAcc, "getregs in remotegdb, size %lu\n", size());
    for (int i = 0; i < NumIntArchRegs; i++)
        r.gpr[i] = context->readIntReg(i);
    r.pc = context->pcState().pc();
    for (int i = 0; i < NumFloatRegs; i++)
        r.fpr[i] = context->readFloatReg(i);

    // TODO schi
    // r.csr_base = context->readMiscReg(0);
    r.fflags = context->readMiscReg(MISCREG_FFLAGS);
    r.frm = context->readMiscReg(MISCREG_FRM);
    r.fcsr = context->readMiscReg(MISCREG_FCSR);
    /* TODO schi
    for (int i = ExplicitCSRs; i < NumMiscRegs - 1; i++)
        r.csr[i - ExplicitCSRs] = context->readMiscReg(i);
        */
}

void
RemoteGDB::PpuGdbRegCache::setRegs(ThreadContext *context_) const
{
    PpuThreadContext* context = dynamic_cast<PpuThreadContext*>(context_);
    DPRINTF(PpuGDBAcc, "setregs in remotegdb \n");
    for (int i = 0; i < NumIntArchRegs; i++)
        context->setIntReg(i, r.gpr[i]);
    context->pcState(r.pc);
    for (int i = 0; i < NumFloatRegs; i++)
        context->setFloatReg(i, r.fpr[i]);

    // TODO schi
    // context->setMiscReg(0, r.csr_base);
    context->setMiscReg(MISCREG_FFLAGS, r.fflags);
    context->setMiscReg(MISCREG_FRM, r.frm);
    context->setMiscReg(MISCREG_FCSR, r.fcsr);
    /* TODO schi
    for (int i = ExplicitCSRs; i < NumMiscRegs; i++)
        context->setMiscReg(i, r.csr[i - ExplicitCSRs]);
        */
}

void
RemoteGDB::Ppu32GdbRegCache::getRegs(ThreadContext *context_)
{
    PpuThreadContext* context = dynamic_cast<PpuThreadContext*>(context_);
    DPRINTF(PpuGDBAcc, "getregs in remotegdb\n");

    // read pc
    r.pc = context->pcState().pc();
    DPRINTF(PpuGDBAcc, "current pc: %#x\n", r.pc);
    // read general purpose registers
    for (int i = 0; i < NumIntArchRegs; i++) {
        r.gpr[i] = context->readIntReg(i);
    }
    // read floating point registers
    /*
    for (int i = 0; i < NumFloatRegs; i++) {
        r.fpr[i] = context->readFloatReg(i);
    }

    r.dcsr = context->readMiscReg(MISCREG_DCSR);
    r.dpc = context->readMiscReg(MISCREG_DPC);
    r.dscratch = context->readMiscReg(MISCREG_DSCRATCH);
    */
}

void
RemoteGDB::Ppu32GdbRegCache::setRegs(ThreadContext *context_) const
{
    PpuThreadContext* context = dynamic_cast<PpuThreadContext*>(context_);
    DPRINTF(PpuGDBAcc, "setregs in remotegdb\n");

    // set pc
    context->pcState(r.pc);
    // set general purpose registers
    for (int i = 0; i < NumIntArchRegs; i++) {
        context->setIntReg(i, r.gpr[i]);
    }
}

PpuBaseGdbRegCache*
RemoteGDB::gdbRegs()
{
    if (isRv32(context())) {
    // if (dynamic_cast<PpuFsWorkload*>(system())->rv32()) {
        return &regCache32;
    } else {
        return &regCache;
    }
    // return &regCache;
}

} // PpuISA
