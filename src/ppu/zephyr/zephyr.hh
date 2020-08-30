/*
 * Copyright (c) 2015 ARM Limited
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
 *
 * Authors: Erik Hallnor
 *          Steve Reinhardt
 *          Andreas Hansson
 */

#ifndef __CPU_ZEPHYR_ZEPHYR_HH__
#define __CPU_ZEPHYR_ZEPHYR_HH__

#include <set>
#include <thread>
#include <unordered_map>

#include "base/statistics.hh"
#include "mem/port.hh"
#include "params/Zephyr.hh"
#include "sim/clocked_object.hh"
#include "sim/eventq.hh"
#include "sim/stats.hh"

/**
 * The Zephyr class tests a cache coherent memory system by
 * generating false sharing and verifying the read data against a
 * reference updated on the completion of writes. Each tester reads
 * and writes a specific byte in a cache line, as determined by its
 * unique id. Thus, all requests issued by the Zephyr instance are a
 * single byte and a specific address is only ever touched by a single
 * tester.
 *
 * In addition to verifying the data, the tester also has timeouts for
 * both requests and responses, thus checking that the memory-system
 * is making progress.
 */
class Zephyr : public ClockedObject
{

  public:

    typedef ZephyrParams Params;
    Zephyr(const Params *p);

    void regStats() override;

    Port &getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;


    void memAccess( bool cmd_read, uint32_t data, bool uncacheable, Addr paddr, bool functional);

    // store the expected value for the addresses we have touched
    std::unordered_map<Addr, uint32_t> referenceData;


  protected:
    // method call into zephyr os
    void zephyrOs();

    // method to tick interval
    void zephyrOsTick();
    EventFunctionWrapper zephyrOsTickEvent;

    void noRequest();

    EventFunctionWrapper noRequestEvent;

    void noResponse();

    EventFunctionWrapper noResponseEvent;

    class CpuPort : public MasterPort
    {
        Zephyr &zephyr;

      public:

        CpuPort(const std::string &_name, Zephyr &_zephyr)
            : MasterPort(_name, &_zephyr), zephyr(_zephyr)
        { }

      protected:

        bool recvTimingResp(PacketPtr pkt);

        void recvTimingSnoopReq(PacketPtr pkt) { }

        void recvFunctionalSnoop(PacketPtr pkt) { }

        Tick recvAtomicSnoop(PacketPtr pkt) { return 0; }

        void recvReqRetry();
    };

    CpuPort port;

    PacketPtr retryPkt;

    const unsigned size;

    const Cycles interval;

    std::thread *zephyr_thread;
    std::string fileName;

    const unsigned percentReads;
    const unsigned percentFunctional;
    const unsigned percentUncacheable;

    /** Request id for all generated traffic */
    MasterID masterId;

    unsigned int id;

    std::set<Addr> outstandingAddrs;

    const unsigned blockSize;

    const Addr blockAddrMask;

    /**
     * Get the block aligned address.
     *
     * @param addr Address to align
     * @return The block aligned address
     */
    Addr blockAlign(Addr addr) const
    {
        return (addr & ~blockAddrMask);
    }

    Addr baseAddr1;
    Addr baseAddr2;
    Addr uncacheAddr;

    Tick startTick;
    Tick OSEventTick;

    const unsigned progressInterval;  // frequency of progress reports
    const Cycles progressCheck;
    Tick nextProgressMessage;   // access # for next progress report

    uint64_t numReads;
    uint64_t numWrites;
    const uint64_t maxLoads;

    const bool atomic;

    const bool suppressFuncWarnings;

    Stats::Scalar numReadsStat;
    Stats::Scalar numWritesStat;

    /**
     * Complete a request by checking the response.
     *
     * @param pkt Response packet
     * @param functional Whether the access was functional or not
     */
    void completeRequest(PacketPtr pkt, bool functional = false);

    bool sendPkt(PacketPtr pkt);

    void recvRetry();

};

#endif // __CPU_ZEPHYR_ZEPHYR_HH__
