/*
 * Copyright (c) 2012-2014, 2017 ARM Limited
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
 * Authors: Andrew Bardsley
 */

#include "ppu/minor/cpu.hh"

#include "arch/utility.hh"
#include "ppu/minor/dyn_inst.hh"
#include "ppu/minor/fetch1.hh"
#include "ppu/minor/pipeline.hh"
#include "debug/PpuDrain.hh"
#include "debug/PpuMinorCPU.hh"
#include "debug/PpuQuiesce.hh"

MinorPPU::MinorPPU(MinorPPUParams *params) :
    PpuBaseCPU(params),
    threadPolicy(params->threadPolicy)
{
    /* This is only written for one thread at the moment */
    Minor::MinorThread *thread;

    for (ThreadID i = 0; i < numThreads; i++) {
        if (FullSystem) {
            thread = new Minor::MinorThread(this, i, params->system,
                    params->itb, params->dtb, params->isa[i]);
            thread->setStatus(ThreadContext::Halted);
        } else {
            thread = new Minor::MinorThread(this, i, params->system,
                    params->workload[i], params->itb, params->dtb,
                    params->isa[i]);
        }

        threads.push_back(thread);
        ThreadContext *tc = thread->getTC();
        threadContexts.push_back(tc);
    }


    if (params->checker) {
        fatal("The Minor model doesn't support checking (yet)\n");
    }

    Minor::MinorDynInst::init();

    pipeline = new Minor::Pipeline(*this, *params);
    activityRecorder = pipeline->getActivityRecorder();
}

MinorPPU::~MinorPPU()
{
    delete pipeline;

    for (ThreadID thread_id = 0; thread_id < threads.size(); thread_id++) {
        delete threads[thread_id];
    }
}

void
MinorPPU::init()
{
    PpuBaseCPU::init();

    if (!params()->switched_out &&
        system->getMemoryMode() != Enums::timing)
    {
        fatal("The Minor CPU requires the memory system to be in "
            "'timing' mode.\n");
    }

    /* Initialise the ThreadContext's memory proxies */
    for (ThreadID thread_id = 0; thread_id < threads.size(); thread_id++) {
        ThreadContext *tc = getContext(thread_id);

        tc->initMemProxies(tc);
    }
}

/** Stats interface from SimObject (by way of PpuBaseCPU) */
void
MinorPPU::regStats()
{
    PpuBaseCPU::regStats();
    stats.regStats(name(), *this);
    pipeline->regStats();
}

void
MinorPPU::serializeThread(CheckpointOut &cp, ThreadID thread_id) const
{
    threads[thread_id]->serialize(cp);
}

void
MinorPPU::unserializeThread(CheckpointIn &cp, ThreadID thread_id)
{
    threads[thread_id]->unserialize(cp);
}

void
MinorPPU::serialize(CheckpointOut &cp) const
{
    pipeline->serialize(cp);
    PpuBaseCPU::serialize(cp);
}

void
MinorPPU::unserialize(CheckpointIn &cp)
{
    pipeline->unserialize(cp);
    PpuBaseCPU::unserialize(cp);
}

Addr
MinorPPU::dbg_vtophys(Addr addr)
{
    /* Note that this gives you the translation for thread 0 */
    panic("No implementation for vtophy\n");

    return 0;
}

void
MinorPPU::wakeup(ThreadID tid)
{
    DPRINTF(PpuDrain, "[tid:%d] MinorPPU wakeup\n", tid);
    assert(tid < numThreads);

    if (threads[tid]->status() == ThreadContext::Suspended) {
        threads[tid]->activate();
    }
}

void
MinorPPU::startup()
{
    DPRINTF(PpuMinorCPU, "MinorPPU startup\n");

    PpuBaseCPU::startup();

    for (ThreadID tid = 0; tid < numThreads; tid++) {
        threads[tid]->startup();
        pipeline->wakeupFetch(tid);
    }
}

DrainState
MinorPPU::drain()
{
    // Deschedule any power gating event (if any)
    deschedulePowerGatingEvent();

    if (switchedOut()) {
        DPRINTF(PpuDrain, "Minor CPU switched out, draining not needed.\n");
        return DrainState::Drained;
    }

    DPRINTF(PpuDrain, "MinorPPU drain\n");

    /* Need to suspend all threads and wait for Execute to idle.
     * Tell Fetch1 not to fetch */
    if (pipeline->drain()) {
        DPRINTF(PpuDrain, "MinorPPU drained\n");
        return DrainState::Drained;
    } else {
        DPRINTF(PpuDrain, "MinorPPU not finished draining\n");
        return DrainState::Draining;
    }
}

void
MinorPPU::signalDrainDone()
{
    DPRINTF(PpuDrain, "MinorPPU drain done\n");
    Drainable::signalDrainDone();
}

void
MinorPPU::drainResume()
{
    /* When taking over from another cpu make sure lastStopped
     * is reset since it might have not been defined previously
     * and might lead to a stats corruption */
    pipeline->resetLastStopped();

    if (switchedOut()) {
        DPRINTF(PpuDrain, "drainResume while switched out.  Ignoring\n");
        return;
    }

    DPRINTF(PpuDrain, "MinorPPU drainResume\n");

    if (!system->isTimingMode()) {
        fatal("The Minor CPU requires the memory system to be in "
            "'timing' mode.\n");
    }

    for (ThreadID tid = 0; tid < numThreads; tid++){
        wakeup(tid);
    }

    pipeline->drainResume();

    // Reschedule any power gating event (if any)
    schedulePowerGatingEvent();
}

void
MinorPPU::memWriteback()
{
    DPRINTF(PpuDrain, "MinorPPU memWriteback\n");
}

void
MinorPPU::switchOut()
{
    DPRINTF(PpuMinorCPU, "MinorPPU switchOut\n");

    assert(!switchedOut());
    PpuBaseCPU::switchOut();

    /* Check that the CPU is drained? */
    activityRecorder->reset();
}

void
MinorPPU::takeOverFrom(PpuBaseCPU *old_cpu)
{
    DPRINTF(PpuMinorCPU, "MinorPPU takeOverFrom\n");

    PpuBaseCPU::takeOverFrom(old_cpu);
}

void
MinorPPU::activateContext(ThreadID thread_id)
{
    DPRINTF(PpuMinorCPU, "ActivateContext thread: %d\n", thread_id);

    /* Do some cycle accounting.  lastStopped is reset to stop the
     *  wakeup call on the pipeline from adding the quiesce period
     *  to PpuBaseCPU::numCycles */
    stats.quiesceCycles += pipeline->cyclesSinceLastStopped();
    pipeline->resetLastStopped();

    /* Wake up the thread, wakeup the pipeline tick */
    threads[thread_id]->activate();
    wakeupOnEvent(Minor::Pipeline::CPUStageId);
    pipeline->wakeupFetch(thread_id);

    PpuBaseCPU::activateContext(thread_id);
}

void
MinorPPU::suspendContext(ThreadID thread_id)
{
    DPRINTF(PpuMinorCPU, "SuspendContext %d\n", thread_id);

    threads[thread_id]->suspend();

    PpuBaseCPU::suspendContext(thread_id);
}

void
MinorPPU::wakeupOnEvent(unsigned int stage_id)
{
    DPRINTF(PpuQuiesce, "Event wakeup from stage %d\n", stage_id);

    /* Mark that some activity has taken place and start the pipeline */
    activityRecorder->activateStage(stage_id);
    pipeline->start();
}

MinorPPU *
MinorPPUParams::create()
{
    return new MinorPPU(this);
}

Port &
MinorPPU::getInstPort()
{
    return pipeline->getInstPort();
}

Port &
MinorPPU::getDataPort()
{
    return pipeline->getDataPort();
}

Counter
MinorPPU::totalInsts() const
{
    Counter ret = 0;

    for (auto i = threads.begin(); i != threads.end(); i ++)
        ret += (*i)->numInst;

    return ret;
}

Counter
MinorPPU::totalOps() const
{
    Counter ret = 0;

    for (auto i = threads.begin(); i != threads.end(); i ++)
        ret += (*i)->numOp;

    return ret;
}
