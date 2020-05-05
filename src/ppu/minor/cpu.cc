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

PpuMinorPPU::PpuMinorPPU(PpuMinorPPUParams *params) :
    PpuBaseCPU(params),
    threadPolicy(params->threadPolicy)
{
    /* This is only written for one thread at the moment */
    PpuMinor::PpuMinorThread *thread;

    for (ThreadID i = 0; i < numThreads; i++) {
        if (PpuFullSystem) {
            thread = new PpuMinor::PpuMinorThread(this, i, params->system,
                    params->itb, params->dtb, params->isa[i]);
            thread->setStatus(PpuThreadContext::Halted);
        } else {
            panic("Don't support SE mode in MiniorCPU\n");
            /*
            thread = new PpuMinor::PpuMinorThread(this, i, params->system,
                    params->workload[i], params->itb, params->dtb,
                    params->isa[i]);
                    */
        }

        threads.push_back(thread);
        PpuThreadContext *tc = thread->getTC();
        threadContexts.push_back(tc);
    }


    if (params->checker) {
        fatal("The PpuMinor model doesn't support checking (yet)\n");
    }

    PpuMinor::PpuMinorDynInst::init();

    pipeline = new PpuMinor::Pipeline(*this, *params);
    activityRecorder = pipeline->getActivityRecorder();
}

PpuMinorPPU::~PpuMinorPPU()
{
    delete pipeline;

    for (ThreadID thread_id = 0; thread_id < threads.size(); thread_id++) {
        delete threads[thread_id];
    }
}

void
PpuMinorPPU::init()
{
    PpuBaseCPU::init();

    if (!params()->switched_out &&
        system->getMemoryMode() != Enums::timing)
    {
        fatal("The PpuMinor CPU requires the memory system to be in "
            "'timing' mode.\n");
    }

    /* Initialise the PpuThreadContext's memory proxies */
    for (ThreadID thread_id = 0; thread_id < threads.size(); thread_id++) {
        PpuThreadContext *tc = dynamic_cast<PpuThreadContext*>(getContext(thread_id));

        tc->initMemProxies(tc);
    }
}

/** Stats interface from SimObject (by way of PpuBaseCPU) */
void
PpuMinorPPU::regStats()
{
    PpuBaseCPU::regStats();
    stats.regStats(name(), *this);
    pipeline->regStats();
}

void
PpuMinorPPU::serializeThread(CheckpointOut &cp, ThreadID thread_id) const
{
    threads[thread_id]->serialize(cp);
}

void
PpuMinorPPU::unserializeThread(CheckpointIn &cp, ThreadID thread_id)
{
    threads[thread_id]->unserialize(cp);
}

void
PpuMinorPPU::serialize(CheckpointOut &cp) const
{
    pipeline->serialize(cp);
    PpuBaseCPU::serialize(cp);
}

void
PpuMinorPPU::unserialize(CheckpointIn &cp)
{
    pipeline->unserialize(cp);
    PpuBaseCPU::unserialize(cp);
}

Addr
PpuMinorPPU::dbg_vtophys(Addr addr)
{
    /* Note that this gives you the translation for thread 0 */
    panic("No implementation for vtophy\n");

    return 0;
}

void
PpuMinorPPU::wakeup(ThreadID tid)
{
    DPRINTF(PpuDrain, "[tid:%d] PpuMinorPPU wakeup\n", tid);
    assert(tid < numThreads);

    if (threads[tid]->status() == PpuThreadContext::Suspended) {
        threads[tid]->activate();
    }
}

void
PpuMinorPPU::startup()
{
    DPRINTF(PpuMinorCPU, "PpuMinorPPU startup\n");

    PpuBaseCPU::startup();

    for (ThreadID tid = 0; tid < numThreads; tid++) {
        threads[tid]->startup();
        pipeline->wakeupFetch(tid);
    }
}

DrainState
PpuMinorPPU::drain()
{
    // Deschedule any power gating event (if any)
    deschedulePowerGatingEvent();

    if (switchedOut()) {
        DPRINTF(PpuDrain, "PpuMinor CPU switched out, draining not needed.\n");
        return DrainState::Drained;
    }

    DPRINTF(PpuDrain, "PpuMinorPPU drain\n");

    /* Need to suspend all threads and wait for Execute to idle.
     * Tell Fetch1 not to fetch */
    if (pipeline->drain()) {
        DPRINTF(PpuDrain, "PpuMinorPPU drained\n");
        return DrainState::Drained;
    } else {
        DPRINTF(PpuDrain, "PpuMinorPPU not finished draining\n");
        return DrainState::Draining;
    }
}

void
PpuMinorPPU::signalDrainDone()
{
    DPRINTF(PpuDrain, "PpuMinorPPU drain done\n");
    Drainable::signalDrainDone();
}

void
PpuMinorPPU::drainResume()
{
    /* When taking over from another cpu make sure lastStopped
     * is reset since it might have not been defined previously
     * and might lead to a stats corruption */
    pipeline->resetLastStopped();

    if (switchedOut()) {
        DPRINTF(PpuDrain, "drainResume while switched out.  Ignoring\n");
        return;
    }

    DPRINTF(PpuDrain, "PpuMinorPPU drainResume\n");

    if (!system->isTimingMode()) {
        fatal("The PpuMinor CPU requires the memory system to be in "
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
PpuMinorPPU::memWriteback()
{
    DPRINTF(PpuDrain, "PpuMinorPPU memWriteback\n");
}

void
PpuMinorPPU::switchOut()
{
    DPRINTF(PpuMinorCPU, "PpuMinorPPU switchOut\n");

    assert(!switchedOut());
    PpuBaseCPU::switchOut();

    /* Check that the CPU is drained? */
    activityRecorder->reset();
}

void
PpuMinorPPU::takeOverFrom(PpuBaseCPU *old_cpu)
{
    DPRINTF(PpuMinorCPU, "PpuMinorPPU takeOverFrom\n");

    PpuBaseCPU::takeOverFrom(old_cpu);
}

void
PpuMinorPPU::activateContext(ThreadID thread_id)
{
    DPRINTF(PpuMinorCPU, "ActivateContext thread: %d\n", thread_id);

    /* Do some cycle accounting.  lastStopped is reset to stop the
     *  wakeup call on the pipeline from adding the quiesce period
     *  to PpuBaseCPU::numCycles */
    stats.quiesceCycles += pipeline->cyclesSinceLastStopped();
    pipeline->resetLastStopped();

    /* Wake up the thread, wakeup the pipeline tick */
    threads[thread_id]->activate();
    wakeupOnEvent(PpuMinor::Pipeline::CPUStageId);
    pipeline->wakeupFetch(thread_id);

    PpuBaseCPU::activateContext(thread_id);
}

void
PpuMinorPPU::suspendContext(ThreadID thread_id)
{
    DPRINTF(PpuMinorCPU, "SuspendContext %d\n", thread_id);

    threads[thread_id]->suspend();

    PpuBaseCPU::suspendContext(thread_id);
}

void
PpuMinorPPU::wakeupOnEvent(unsigned int stage_id)
{
    DPRINTF(PpuQuiesce, "Event wakeup from stage %d\n", stage_id);

    /* Mark that some activity has taken place and start the pipeline */
    activityRecorder->activateStage(stage_id);
    pipeline->start();
}

PpuMinorPPU *
PpuMinorPPUParams::create()
{
    return new PpuMinorPPU(this);
}

Port &
PpuMinorPPU::getInstPort()
{
    return pipeline->getInstPort();
}

Port &
PpuMinorPPU::getDataPort()
{
    return pipeline->getDataPort();
}

Counter
PpuMinorPPU::totalInsts() const
{
    Counter ret = 0;

    for (auto i = threads.begin(); i != threads.end(); i ++)
        ret += (*i)->numInst;

    return ret;
}

Counter
PpuMinorPPU::totalOps() const
{
    Counter ret = 0;

    for (auto i = threads.begin(); i != threads.end(); i ++)
        ret += (*i)->numOp;

    return ret;
}
