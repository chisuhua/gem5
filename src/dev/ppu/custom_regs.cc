/*
 * Copyright (c) 2018 TU Dresden
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
 * Authors: Robert Scheffel
 */

#include "dev/ppu/custom_regs.hh"

#include "mem/packet.hh"
#include "mem/packet_access.hh"

CustomRegs::CustomRegs(Params *p)
    : BasicPioDevice(p, sizeof(p->regs)*4)
{
    for (auto const& addr : p->regs) {
        regmap[addr] = 0;
    }

    pioDelay = clockPeriod();
}

Tick
CustomRegs::read(PacketPtr pkt)
{
    assert(pkt->getAddr() >= pioAddr && pkt->getAddr() < pioAddr + pioSize);

    Addr addr = pkt->getAddr() - pioAddr;
    // FIXME schi setRaw or SetLE?
    // pkt->set<uint32_t>(regmap[addr]);
    pkt->setRaw<uint32_t>(regmap[addr]);

    pkt->makeAtomicResponse();
    return pioDelay;
}

Tick
CustomRegs::write(PacketPtr pkt)
{
    // warn("addr: %#x\n", pkt->getAddr());
    // for (auto const& x: regmap) {
    //     warn("key = %#x", x.first);
    // }
    assert(pkt->getAddr() >= pioAddr && pkt->getAddr() < pioAddr + pioSize);

    Addr addr = pkt->getAddr() - pioAddr;
    // FIXME schi getRaw or getLE
    // regmap[addr] = pkt->get<uint32_t>();
    regmap[addr] = pkt->getRaw<uint32_t>();

    pkt->makeAtomicResponse();
    return pioDelay;
}

CustomRegs *
CustomRegsParams::create()
{
    return new CustomRegs(this);
}