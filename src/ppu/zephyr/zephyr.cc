/*
 * Copyright (c) 2015, 2019 ARM Limited
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

#include "ppu/zephyr/zephyr.hh"

#include "base/random.hh"
#include "base/statistics.hh"
#include "base/trace.hh"
#include "debug/Zephyr.hh"
#include "ppu/zephyr/dso_library.hh"
#include "sim/sim_exit.hh"
#include "sim/stats.hh"
#include "sim/system.hh"

using namespace std;

unsigned int ZEPHYR_ALLOCATOR = 0;

bool
Zephyr::CpuPort::recvTimingResp(PacketPtr pkt)
{
    zephyr.completeRequest(pkt);
    return true;
}

void
Zephyr::CpuPort::recvReqRetry()
{
    zephyr.recvRetry();
}

bool
Zephyr::sendPkt(PacketPtr pkt) {
    if (atomic) {
        port.sendAtomic(pkt);
        completeRequest(pkt);
    } else {
        if (!port.sendTimingReq(pkt)) {
            retryPkt = pkt;
            return false;
        }
    }
    return true;
}

template <typename T>
std::function<T> GetFunction(void * func)
{
    return std::function<T>((T*)(func));
}

static Zephyr *zephyr_instance;
static EventQueue *event_queue;
// __thread std::condition_variable data_read_done;


// TODO add  api to exit sim
static void zephyr_api_exit_sim() {
    exitSimLoop("maximum number of loads reached");
}

static uint32_t zephyr_api_mem_access(Addr paddr, uint32_t data, bool cmd_read, bool uncacheable, bool functional) {
    curEventQueue(event_queue);
    Tick duration_per_access = 2000;
    Tick duration_progress = 100000;
    // Tick now = curTick() + duration_per_access;
    // Tick now = event_queue->nextTick() + duration_per_access;
    Tick now = event_queue->getCurTick() + duration_per_access;

    event_queue->schedule(new EventFunctionWrapper(
                [cmd_read, data, uncacheable, paddr, functional]{
                    zephyr_instance->memAccess(cmd_read, data, uncacheable, paddr, functional);
                }, zephyr_instance->name() + ".memAcess", true
                ),
            now, true);
    now += duration_progress;
    while (true) {
        if (event_queue->empty()){
            break;
        }
        Tick next = event_queue->nextTick();

        if (next <= now) {
            event_queue->serviceEvents(now);
            //event_queue->serviceOne();
            continue;
        }
        break;
    }

    if (cmd_read)
        return zephyr_instance->referenceData[paddr];
    return 0;
}

static void zephyr_api_mem_write(Addr paddr, uint32_t data, bool uncacheable = false) {
    zephyr_api_mem_access(paddr, data, false, uncacheable, false);
}

static uint32_t zephyr_api_mem_read(Addr paddr, bool uncacheable = false) {
    return zephyr_api_mem_access(paddr, 0, true, uncacheable, false);
}


Zephyr::Zephyr(const Params *p)
    : ClockedObject(p),
      zephyrOsEvent([this]{ zephyrOs(); }, name()),
      noRequestEvent([this]{ noRequest(); }, name()),
      noResponseEvent([this]{ noResponse(); }, name()),
      port("port", *this),
      retryPkt(nullptr),
      size(p->size),
      interval(p->interval),
      percentReads(p->percent_reads),
      percentFunctional(p->percent_functional),
      percentUncacheable(p->percent_uncacheable),
      masterId(p->system->getMasterId(this)),
      blockSize(p->system->cacheLineSize()),
      blockAddrMask(p->block_addrmask == 0? blockSize - 1 : p->block_addrmask - 1),
      baseAddr1(p->base_addr1),
      baseAddr2(p->base_addr2),
      uncacheAddr(p->uncache_addr),
      startTick(p->start_tick),
      progressInterval(p->progress_interval),
      progressCheck(p->progress_check),
      nextProgressMessage(p->progress_interval),
      maxLoads(p->max_loads),
      atomic(p->system->isAtomicMode()),
      suppressFuncWarnings(p->suppress_func_warnings)
{
    id = ZEPHYR_ALLOCATOR++;
    fatal_if(id >= blockSize, "Too many testers, only %d allowed\n",
             blockSize - 1);

    // baseAddr1 = baseAddr;
    // baseAddr2 = 0x400000;
    // uncacheAddr = 0x800000;

    // set up counters
    numReads = 0;
    numWrites = 0;


    zephyr_instance = this;
    event_queue = _curEventQueue;

    fileName = p->file_name;


    // kick things into action
    // zephyrOs();
    // schedule(zephyrOsEvent, curTick() + startTick);
    // schedule(noRequestEvent, clockEdge(progressCheck));
    zephyr_thread = new std::thread([this](){ this->zephyrOs();});
    zephyr_thread->detach();
}

static void* gem5api_vector[] = {(void*)zephyr_api_mem_read, (void*)zephyr_api_mem_write, (void*)zephyr_api_exit_sim};

void Zephyr::zephyrOs()
{
    DSOLibrary lib;
    lib.Init(fileName);

    typedef int(*pFunc)(int, char *[]);

    int argc = 2;
    char argv_str[] = "gem5_main";
    // char argv_str2[] = "--gem5api";
    char* argv[] = {argv_str, /*argv_str2,*/ (char*)&gem5api_vector};

    pFunc gem5_main = (pFunc)lib.GetSymbol("gem5_main");

    int zephyr_ret = gem5_main(argc, argv);
    // zephyr_thread = new std::thread(gem5_main, argc, argv);
    DPRINTF(Zephyr, "gem5_main argc=%d, argv=%s, %d\n", argc, argv[0], zephyr_ret);
}

Port &
Zephyr::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "port")
        return port;
    else
        return ClockedObject::getPort(if_name, idx);
}

void
Zephyr::completeRequest(PacketPtr pkt, bool functional)
{
    const RequestPtr &req = pkt->req;
    assert(req->getSize() == 4);        // only 4byte access

    // this address is no longer outstanding
    auto remove_addr = outstandingAddrs.find(req->getPaddr());
    assert(remove_addr != outstandingAddrs.end());
    outstandingAddrs.erase(remove_addr);

    DPRINTF(Zephyr, "Completing %s at address %x (blk %x) %s\n",
            pkt->isWrite() ? "write" : "read",
            req->getPaddr(), blockAlign(req->getPaddr()),
            pkt->isError() ? "error" : "success");

    const uint32_t *pkt_data = pkt->getConstPtr<uint32_t>();

    if (pkt->isError()) {
        if (!functional || !suppressFuncWarnings) {
            warn("%s access failed at %#x\n",
                 pkt->isWrite() ? "Write" : "Read", req->getPaddr());
        }
    } else {
        if (pkt->isRead()) {
            uint32_t ref_data = referenceData[req->getPaddr()];
            if (pkt_data[0] != ref_data) {
                warn("%s: read of %x (blk %x) @ cycle %d "
                      "returns %x, expected %x\n", name(),
                      req->getPaddr(), blockAlign(req->getPaddr()), curTick(),
                      pkt_data[0], ref_data);
                referenceData[req->getPaddr()] = pkt_data[0];
            }

            numReads++;
            numReadsStat++;

            if (numReads == (uint64_t)nextProgressMessage) {
                ccprintf(cerr, "%s: completed %d read, %d write accesses @%d\n",
                         name(), numReads, numWrites, curTick());
                nextProgressMessage += progressInterval;
            }

        } else {
            assert(pkt->isWrite());

            // update the reference data
            referenceData[req->getPaddr()] = pkt_data[0];
            numWrites++;
            numWritesStat++;
        }
    }

    // the packet will delete the data
    delete pkt;

    // finally shift the response timeout forward if we are still
    // expecting responses; deschedule it otherwise
    if (outstandingAddrs.size() != 0)
        reschedule(noResponseEvent, clockEdge(progressCheck));
    else if (noResponseEvent.scheduled())
        deschedule(noResponseEvent);
}

void
Zephyr::regStats()
{
    ClockedObject::regStats();

    using namespace Stats;

    numReadsStat
        .name(name() + ".num_reads")
        .desc("number of read accesses completed")
        ;

    numWritesStat
        .name(name() + ".num_writes")
        .desc("number of write accesses completed")
        ;
}

void
Zephyr::memAccess( bool cmd_read, uint32_t data, bool uncacheable, Addr paddr, bool functional)
{
    // we should never tick if we are waiting for a retry
    assert(!retryPkt);

    // create a new request
    Request::Flags flags;
    Tick t = curTick();

    DPRINTF(Zephyr, "curTick in memAccess is %d", t);
    // generate a unique address

        // use the tester id as offset within the block for false sharing
        // paddr = blockAlign(paddr);

    if (uncacheable) {
        flags.set(Request::UNCACHEABLE);
    }

    bool do_functional = functional && !uncacheable;
    RequestPtr req = std::make_shared<Request>(paddr, 4, flags, masterId); // it is 4Byte access
    req->setContext(id);

    outstandingAddrs.insert(paddr);

    // sanity check
    panic_if(outstandingAddrs.size() > 100,
             "Tester %s has more than 100 outstanding requests\n", name());

    PacketPtr pkt = nullptr;
    // uint8_t *pkt_data = new uint8_t[4];
    uint32_t *pkt_data = new uint32_t[1];

    if (cmd_read) {
        // start by ensuring there is a reference value if we have not
        // seen this address before
        uint32_t M5_VAR_USED ref_data = 0;
        auto ref = referenceData.find(req->getPaddr());
        if (ref == referenceData.end()) {
            referenceData[req->getPaddr()] = 0;
        } else {
            ref_data = ref->second;
        }

        DPRINTF(Zephyr,
                "Initiating %sread at addr %x (blk %x) expecting %x\n",
                do_functional ? "functional " : "", req->getPaddr(),
                blockAlign(req->getPaddr()), ref_data);

        pkt = new Packet(req, MemCmd::ReadReq);
        pkt->dataDynamic(pkt_data);
    } else {
        DPRINTF(Zephyr, "Initiating %swrite at addr %x (blk %x) value %x\n",
                do_functional ? "functional " : "", req->getPaddr(),
                blockAlign(req->getPaddr()), data);

        pkt = new Packet(req, MemCmd::WriteReq);
        pkt->dataDynamic(pkt_data);
        pkt_data[0] = data;
    }

    // there is no point in ticking if we are waiting for a retry
    //bool keep_ticking = true;
    if (do_functional) {
        pkt->setSuppressFuncError();
        port.sendFunctional(pkt);
        completeRequest(pkt, true);
    } else {
        /*keep_ticking = */sendPkt(pkt);
    }

    /*
    if (keep_ticking) {
        // schedule the next tick
        schedule(tickEvent, clockEdge(interval));

        // finally shift the timeout for sending of requests forwards
        // as we have successfully sent a packet
        reschedule(noRequestEvent, clockEdge(progressCheck), true);
    } else {
        DPRINTF(Zephyr, "Waiting for retry\n");
    }
    */

    // Schedule noResponseEvent now if we are expecting a response
    if (!noResponseEvent.scheduled() && (outstandingAddrs.size() != 0))
    {
        event_queue->schedule(&noResponseEvent, clockEdge(progressCheck));
        /*
        event_queue->schedule(new EventFunctionWrapper(
                [](){
                },
                zephyr_instance->name() + ".progressCheck", true
                ),
            clockEdge(progressCheck));
            */
    }

}

void
Zephyr::noRequest()
{
    panic("%s did not send a request for %d cycles", name(), progressCheck);
}

void
Zephyr::noResponse()
{
    // warn("%s did not see a response for %d cycles", name(), progressCheck);
    panic("%s did not see a response for %d cycles", name(), progressCheck);
}

void
Zephyr::recvRetry()
{
    assert(retryPkt);
    if (port.sendTimingReq(retryPkt)) {
        DPRINTF(Zephyr, "Proceeding after successful retry\n");

        retryPkt = nullptr;
        // kick things into action again
        // schedule(tickEvent, clockEdge(interval));
        // reschedule(noRequestEvent, clockEdge(progressCheck), true);
    }
}

Zephyr *
ZephyrParams::create()
{
    return new Zephyr(this);
}
