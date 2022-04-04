/*
 * Copyright (c) 2017 Jason Lowe-Power
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

#include "host_datapath.hh"

#include "base/trace.hh"
#include "debug/HostDataPath.hh"

namespace gem5
{

HostDataPath::HostDataPath(const HostDataPathParams &params) :
    SimObject(params),
    range(params.range),
    dataPort(params.name + ".data_port", this),
    blocked(false)
{
}

Port &
HostDataPath::getPort(const std::string &if_name, PortID idx)
{
    // panic_if(idx != InvalidPortID, "This object doesn't support vector ports");

    // This is the name from the Python SimObject declaration (HostDataPath.py)
    if (if_name == "data_port") {
        return dataPort;
    } else {
        // pass it along to our super class
        panic("port name should be data_port");
        return SimObject::getPort(if_name, idx);
    }
}

void
HostDataPath::CPUSidePort::sendPacket(PacketPtr pkt)
{
    // Note: This flow control is very simple since the memobj is blocking.

    panic_if(blockedPacket != nullptr, "Should never try to send if blocked!");

    // If we can't send the packet across the port, store it for later.
    if (!sendTimingResp(pkt)) {
        blockedPacket = pkt;
    }
}

AddrRangeList
HostDataPath::CPUSidePort::getAddrRanges() const
{
    AddrRangeList ranges;
    ranges.push_back(owner->getAddrRange());
    return ranges;
}

void
HostDataPath::CPUSidePort::trySendRetry()
{
    if (needRetry && blockedPacket == nullptr) {
        // Only send a retry if the port is now completely free
        needRetry = false;
        DPRINTF(HostDataPath, "Sending retry req for %d\n", id);
        sendRetryReq();
    }
}

void
HostDataPath::CPUSidePort::recvFunctional(PacketPtr pkt)
{
    // Just forward to the memobj.
    return owner->handleFunctional(pkt);
}

bool
HostDataPath::CPUSidePort::recvTimingReq(PacketPtr pkt)
{
    // Just forward to the memobj.
    if (!owner->handleRequest(pkt)) {
        needRetry = true;
        return false;
    } else {
        return true;
    }
}

void
HostDataPath::CPUSidePort::recvRespRetry()
{
    // We should have a blocked packet if this function is called.
    assert(blockedPacket != nullptr);

    // Grab the blocked packet.
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;

    // Try to resend it. It's possible that it fails again.
    sendPacket(pkt);
}


bool
HostDataPath::handleRequest(PacketPtr pkt)
{
    if (blocked) {
        // There is currently an outstanding request. Stall.
        return false;
    }

    DPRINTF(HostDataPath, "Got request for addr %#x\n", pkt->getAddr());

    // This memobj is now blocked waiting for the response to this packet.
    blocked = true;

    if (pkt->isRead()) {
        const uint8_t *host_addr = reinterpret_cast<const uint8_t*>(pkt->getAddr());
        pkt->setData(host_addr);
    } else {
        uint8_t *host_addr = reinterpret_cast<uint8_t*>(pkt->getAddr());
        pkt->writeData(host_addr);
    }
    pkt->makeResponse();

    return handleResponse(pkt);
}

bool
HostDataPath::handleResponse(PacketPtr pkt)
{
    assert(blocked);
    DPRINTF(HostDataPath, "Got response for addr %#x\n", pkt->getAddr());

    // The packet is now done. We're about to put it in the port, no need for
    // this object to continue to stall.
    // We need to free the resource before sending the packet in case the CPU
    // tries to send another request immediately (e.g., in the same callchain).
    blocked = false;

    // Simply forward to the memory port
    dataPort.sendPacket(pkt);

    // For each of the cpu ports, if it needs to send a retry, it should do it
    // now since this memory object may be unblocked now.
    dataPort.trySendRetry();

    return true;
}

void
HostDataPath::handleFunctional(PacketPtr pkt)
{
    // Just pass this on to the memory side to handle for now.
    // memPort.sendFunctional(pkt);
}

AddrRange
HostDataPath::getAddrRange() const
{
    DPRINTF(HostDataPath, "Sending new ranges\n");
    // Just use the same ranges as whatever is on the memory side.
    //return memPort.getAddrRanges();
    return range;
}

void
HostDataPath::sendRangeChange()
{
    dataPort.sendRangeChange();
}

} // namespace gem5
