/*
 * Copyright (c) 2011-2012, 2016-2018 ARM Limited
 * Copyright (c) 2013 Advanced Micro Devices, Inc.
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
 * Copyright (c) 2001-2006 The Regents of The University of Michigan
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

#ifndef __PPU_SIMPLE_THREAD_HH__
#define __PPU_SIMPLE_THREAD_HH__

#include <array>

#include "arch/decoder.hh"
#include "arch/generic/tlb.hh"
#include "arch/isa.hh"
#include "arch/isa_traits.hh"
#include "arch/registers.hh"
#include "arch/types.hh"
#include "base/types.hh"
#include "config/the_isa.hh"
#include "ppu/thread_context.hh"
#include "ppu/thread_state.hh"
#include "debug/PpuCCRegs.hh"
#include "debug/PpuFloatRegs.hh"
#include "debug/PpuIntRegs.hh"
#include "debug/PpuVecPredRegs.hh"
#include "debug/PpuVecRegs.hh"
#include "mem/page_table.hh"
#include "mem/request.hh"
#include "sim/byteswap.hh"
#include "sim/eventq.hh"
#include "sim/full_system.hh"
// #include "ppu_sim/process.hh"
#include "sim/serialize.hh"
#include "ppu_sim/system.hh"

class PpuThreadContext;


class PpuBaseCPU;
// class PpuCheckerCPU;

class FunctionProfile;
class ProfileNode;

namespace Kernel {
    class Statistics;
}

namespace PpuISA {
/**
 * The SimpleThread object provides a combination of the ThreadState
 * object and the PpuThreadContext interface. It implements the
 * PpuThreadContext interface and adds to the ThreadState object by adding all
 * the objects needed for simple functional execution, including a
 * simple architectural register file, and pointers to the ITB and DTB
 * in full system mode. For CPU models that do not need more advanced
 * ways to hold state (i.e. a separate physical register file, or
 * separate fetch and commit PC's), this SimpleThread class provides
 * all the necessary state for full architecture-level functional
 * simulation.  See the AtomicSimpleCPU or TimingSimpleCPU for
 * examples.
 */

class SimpleThread : public ThreadState, public PpuThreadContext
{
  protected:
    typedef ThePpuISA::MachInst MachInst;
    using VecRegContainer = ThePpuISA::VecRegContainer;
    using VecElem = ThePpuISA::VecElem;
    using VecPredRegContainer = ThePpuISA::VecPredRegContainer;
  public:
    typedef PpuThreadContext::Status Status;

  protected:
    std::array<RegVal, ThePpuISA::NumFloatRegs> floatRegs;
    std::array<RegVal, ThePpuISA::NumIntRegs> intRegs;
    std::array<VecRegContainer, ThePpuISA::NumVecRegs> vecRegs;
    std::array<VecPredRegContainer, ThePpuISA::NumVecPredRegs> vecPredRegs;
    std::array<RegVal, ThePpuISA::NumCCRegs> ccRegs;
    ThePpuISA::ISA *const isa;    // one "instance" of the current ISA.

    ThePpuISA::PCState _pcState;

    /** Did this instruction execute or is it predicated false */
    bool predicate;

    /** True if the memory access should be skipped for this instruction */
    bool memAccPredicate;

  public:
    std::string name() const
    {
        return csprintf("%s.[tid:%i]", baseCpu->name(), threadId());
    }

    PpuPCEventQueue pcEventQueue;
    /**
     * An instruction-based event queue. Used for scheduling events based on
     * number of instructions committed.
     */
    EventQueue comInstEventQueue;

    PpuSOCSystem *system;

    BaseTLB *itb;
    BaseTLB *dtb;

    ThePpuISA::Decoder decoder;

    // constructor: initialize SimpleThread from given process structure
    // FS
    SimpleThread(PpuBaseCPU *_cpu, int _thread_num, PpuSOCSystem *_system,
                 BaseTLB *_itb, BaseTLB *_dtb, ThePpuISA::ISA *_isa,
                 bool use_kernel_stats = true);
    // SE
    SimpleThread(PpuBaseCPU *_cpu, int _thread_num, PpuSOCSystem *_system,
                 PpuSOCProcess *_process, BaseTLB *_itb, BaseTLB *_dtb,
                 ThePpuISA::ISA *_isa);

    virtual ~SimpleThread() {}

    void takeOverFrom(PpuThreadContext *oldContext) override;

    void regStats(const std::string &name) override;

    void copyState(PpuThreadContext *oldContext);

    void serialize(CheckpointOut &cp) const override;
    void unserialize(CheckpointIn &cp) override;
    void startup();

    /***************************************************************
     *  SimpleThread functions to provide CPU with access to various
     *  state.
     **************************************************************/

    /** Returns the pointer to this SimpleThread's PpuThreadContext. Used
     *  when a PpuThreadContext must be passed to objects outside of the
     *  CPU.
     */
    PpuThreadContext *getTC() { return this; }

    void demapPage(Addr vaddr, uint64_t asn)
    {
        itb->demapPage(vaddr, asn);
        dtb->demapPage(vaddr, asn);
    }

    void demapInstPage(Addr vaddr, uint64_t asn)
    {
        itb->demapPage(vaddr, asn);
    }

    void demapDataPage(Addr vaddr, uint64_t asn)
    {
        dtb->demapPage(vaddr, asn);
    }

    void dumpFuncProfile() override;

    /*******************************************
     * PpuThreadContext interface functions.
     ******************************************/

    bool schedule(PpuPCEvent *e) override { return pcEventQueue.schedule(e); }
    bool remove(PpuPCEvent *e) override { return pcEventQueue.remove(e); }

    void
    scheduleInstCountEvent(Event *event, Tick count) override
    {
        comInstEventQueue.schedule(event, count);
    }
    void
    descheduleInstCountEvent(Event *event) override
    {
        comInstEventQueue.deschedule(event);
    }
    Tick
    getCurrentInstCount() override
    {
        return comInstEventQueue.getCurTick();
    }

    PpuBaseCPU *PpugetCpuPtr() override { return baseCpu; }

    int cpuId() const override { return ThreadState::cpuId(); }
    uint32_t socketId() const override { return ThreadState::socketId(); }
    int threadId() const override { return ThreadState::threadId(); }
    void setThreadId(int id) override { ThreadState::setThreadId(id); }
    ContextID contextId() const override { return ThreadState::contextId(); }
    void setContextId(ContextID id) override { ThreadState::setContextId(id); }

    BaseTLB *getITBPtr() override { return itb; }

    BaseTLB *getDTBPtr() override { return dtb; }

    // PpuCheckerCPU *PpugetCheckerCpuPtr() override { return NULL; }
    // CheckerCPU *getCheckerCpuPtr() override { return NULL; }

    BaseISA *getIsaPtr() override { return isa; }

    ThePpuISA::Decoder *getDecoderPtr() override { return &decoder; }

    PpuSOCSystem *PpugetSystemPtr() override { return system; }

    Kernel::Statistics *
    getKernelStats() override
    {
        return ThreadState::getKernelStats();
    }

    PortProxy &getPhysProxy() override { return ThreadState::getPhysProxy(); }
    PortProxy &getVirtProxy() override { return ThreadState::getVirtProxy(); }

    void initMemProxies(PpuThreadContext *tc) override
    {
        ThreadState::initMemProxies(tc);
    }

    // Process *getProcessPtr() override { return ThreadState::getProcessPtr(); }
    PpuSOCProcess *PpugetProcessPtr() override { return ThreadState::getProcessPtr(); }

    void setProcessPtr(Process *p) override {
        panic("ppu simple thread should use PpuSOCProcess instead\n");
        // ThreadState::setProcessPtr(p);
    }
    void setProcessPtr(PpuSOCProcess *p) override { ThreadState::setProcessPtr(p); }

    Status status() const override { return _status; }

    void setStatus(Status newStatus) override { _status = newStatus; }

    /// Set the status to Active.
    void activate() override;

    /// Set the status to Suspended.
    void suspend() override;

    /// Set the status to Halted.
    void halt() override;

    PpuEndQuiesceEvent *
    PpugetQuiesceEvent() override
    {
        return ThreadState::PpugetQuiesceEvent();
    }

    Tick
    readLastActivate() override
    {
        return ThreadState::readLastActivate();
    }
    Tick
    readLastSuspend() override
    {
        return ThreadState::readLastSuspend();
    }

    void profileClear() override { ThreadState::profileClear(); }
    void profileSample() override { ThreadState::profileSample(); }

    void copyArchRegs(PpuThreadContext *tc) override;

    void
    clearArchRegs() override
    {
        _pcState = 0;
        intRegs.fill(0);
        floatRegs.fill(0);
        for (auto &vec_reg: vecRegs)
            vec_reg.zero();
        for (auto &pred_reg: vecPredRegs)
            pred_reg.reset();
        ccRegs.fill(0);
        isa->clear(this);
    }

    //
    // New accessors for new decoder.
    //
    RegVal
    readIntReg(RegIndex reg_idx) const override
    {
        int flatIndex = isa->flattenIntIndex(reg_idx);
        assert(flatIndex < ThePpuISA::NumIntRegs);
        uint64_t regVal(readIntRegFlat(flatIndex));
        DPRINTF(PpuIntRegs, "Reading int reg %d (%d) as %#x.\n",
                reg_idx, flatIndex, regVal);
        return regVal;
    }

    RegVal
    readFloatReg(RegIndex reg_idx) const override
    {
        int flatIndex = isa->flattenFloatIndex(reg_idx);
        assert(flatIndex < ThePpuISA::NumFloatRegs);
        RegVal regVal(readFloatRegFlat(flatIndex));
        DPRINTF(PpuFloatRegs, "Reading float reg %d (%d) bits as %#x.\n",
                reg_idx, flatIndex, regVal);
        return regVal;
    }

    const VecRegContainer&
    readVecReg(const RegId& reg) const override
    {
        int flatIndex = isa->flattenVecIndex(reg.index());
        assert(flatIndex < ThePpuISA::NumVecRegs);
        const VecRegContainer& regVal = readVecRegFlat(flatIndex);
        DPRINTF(PpuVecRegs, "Reading vector reg %d (%d) as %s.\n",
                reg.index(), flatIndex, regVal.print());
        return regVal;
    }

    VecRegContainer&
    getWritableVecReg(const RegId& reg) override
    {
        int flatIndex = isa->flattenVecIndex(reg.index());
        assert(flatIndex < ThePpuISA::NumVecRegs);
        VecRegContainer& regVal = getWritableVecRegFlat(flatIndex);
        DPRINTF(PpuVecRegs, "Reading vector reg %d (%d) as %s for modify.\n",
                reg.index(), flatIndex, regVal.print());
        return regVal;
    }

    /** Vector Register Lane Interfaces. */
    /** @{ */
    /** Reads source vector <T> operand. */
    template <typename T>
    VecLaneT<T, true>
    readVecLane(const RegId& reg) const
    {
        int flatIndex = isa->flattenVecIndex(reg.index());
        assert(flatIndex < ThePpuISA::NumVecRegs);
        auto regVal = readVecLaneFlat<T>(flatIndex, reg.elemIndex());
        DPRINTF(PpuVecRegs, "Reading vector lane %d (%d)[%d] as %lx.\n",
                reg.index(), flatIndex, reg.elemIndex(), regVal);
        return regVal;
    }

    /** Reads source vector 8bit operand. */
    virtual ConstVecLane8
    readVec8BitLaneReg(const RegId &reg) const override
    {
        return readVecLane<uint8_t>(reg);
    }

    /** Reads source vector 16bit operand. */
    virtual ConstVecLane16
    readVec16BitLaneReg(const RegId &reg) const override
    {
        return readVecLane<uint16_t>(reg);
    }

    /** Reads source vector 32bit operand. */
    virtual ConstVecLane32
    readVec32BitLaneReg(const RegId &reg) const override
    {
        return readVecLane<uint32_t>(reg);
    }

    /** Reads source vector 64bit operand. */
    virtual ConstVecLane64
    readVec64BitLaneReg(const RegId &reg) const override
    {
        return readVecLane<uint64_t>(reg);
    }

    /** Write a lane of the destination vector register. */
    template <typename LD>
    void
    setVecLaneT(const RegId &reg, const LD &val)
    {
        int flatIndex = isa->flattenVecIndex(reg.index());
        assert(flatIndex < ThePpuISA::NumVecRegs);
        setVecLaneFlat(flatIndex, reg.elemIndex(), val);
        DPRINTF(PpuVecRegs, "Reading vector lane %d (%d)[%d] to %lx.\n",
                reg.index(), flatIndex, reg.elemIndex(), val);
    }
    virtual void
    setVecLane(const RegId &reg, const LaneData<LaneSize::Byte> &val) override
    {
        return setVecLaneT(reg, val);
    }
    virtual void
    setVecLane(const RegId &reg,
               const LaneData<LaneSize::TwoByte> &val) override
    {
        return setVecLaneT(reg, val);
    }
    virtual void
    setVecLane(const RegId &reg,
               const LaneData<LaneSize::FourByte> &val) override
    {
        return setVecLaneT(reg, val);
    }
    virtual void
    setVecLane(const RegId &reg,
               const LaneData<LaneSize::EightByte> &val) override
    {
        return setVecLaneT(reg, val);
    }
    /** @} */

    const VecElem &
    readVecElem(const RegId &reg) const override
    {
        int flatIndex = isa->flattenVecElemIndex(reg.index());
        assert(flatIndex < ThePpuISA::NumVecRegs);
        const VecElem& regVal = readVecElemFlat(flatIndex, reg.elemIndex());
        DPRINTF(PpuVecRegs, "Reading element %d of vector reg %d (%d) as"
                " %#x.\n", reg.elemIndex(), reg.index(), flatIndex, regVal);
        return regVal;
    }

    const VecPredRegContainer &
    readVecPredReg(const RegId &reg) const override
    {
        int flatIndex = isa->flattenVecPredIndex(reg.index());
        assert(flatIndex < ThePpuISA::NumVecPredRegs);
        const VecPredRegContainer& regVal = readVecPredRegFlat(flatIndex);
        DPRINTF(PpuVecPredRegs, "Reading predicate reg %d (%d) as %s.\n",
                reg.index(), flatIndex, regVal.print());
        return regVal;
    }

    VecPredRegContainer &
    getWritableVecPredReg(const RegId &reg) override
    {
        int flatIndex = isa->flattenVecPredIndex(reg.index());
        assert(flatIndex < ThePpuISA::NumVecPredRegs);
        VecPredRegContainer& regVal = getWritableVecPredRegFlat(flatIndex);
        DPRINTF(PpuVecPredRegs,
                "Reading predicate reg %d (%d) as %s for modify.\n",
                reg.index(), flatIndex, regVal.print());
        return regVal;
    }

    RegVal
    readCCReg(RegIndex reg_idx) const override
    {
        int flatIndex = isa->flattenCCIndex(reg_idx);
        assert(0 <= flatIndex);
        assert(flatIndex < ThePpuISA::NumCCRegs);
        uint64_t regVal(readCCRegFlat(flatIndex));
        DPRINTF(PpuCCRegs, "Reading CC reg %d (%d) as %#x.\n",
                reg_idx, flatIndex, regVal);
        return regVal;
    }

    void
    setIntReg(RegIndex reg_idx, RegVal val) override
    {
        int flatIndex = isa->flattenIntIndex(reg_idx);
        assert(flatIndex < ThePpuISA::NumIntRegs);
        DPRINTF(PpuIntRegs, "Setting int reg %d (%d) to %#x.\n",
                reg_idx, flatIndex, val);
        setIntRegFlat(flatIndex, val);
    }

    void
    setFloatReg(RegIndex reg_idx, RegVal val) override
    {
        int flatIndex = isa->flattenFloatIndex(reg_idx);
        assert(flatIndex < ThePpuISA::NumFloatRegs);
        // XXX: Fix array out of bounds compiler error for gem5.fast
        // when checkercpu enabled
        if (flatIndex < ThePpuISA::NumFloatRegs)
            setFloatRegFlat(flatIndex, val);
        DPRINTF(PpuFloatRegs, "Setting float reg %d (%d) bits to %#x.\n",
                reg_idx, flatIndex, val);
    }

    void
    setVecReg(const RegId &reg, const VecRegContainer &val) override
    {
        int flatIndex = isa->flattenVecIndex(reg.index());
        assert(flatIndex < ThePpuISA::NumVecRegs);
        setVecRegFlat(flatIndex, val);
        DPRINTF(PpuVecRegs, "Setting vector reg %d (%d) to %s.\n",
                reg.index(), flatIndex, val.print());
    }

    void
    setVecElem(const RegId &reg, const VecElem &val) override
    {
        int flatIndex = isa->flattenVecElemIndex(reg.index());
        assert(flatIndex < ThePpuISA::NumVecRegs);
        setVecElemFlat(flatIndex, reg.elemIndex(), val);
        DPRINTF(PpuVecRegs, "Setting element %d of vector reg %d (%d) to"
                " %#x.\n", reg.elemIndex(), reg.index(), flatIndex, val);
    }

    void
    setVecPredReg(const RegId &reg, const VecPredRegContainer &val) override
    {
        int flatIndex = isa->flattenVecPredIndex(reg.index());
        assert(flatIndex < ThePpuISA::NumVecPredRegs);
        setVecPredRegFlat(flatIndex, val);
        DPRINTF(PpuVecPredRegs, "Setting predicate reg %d (%d) to %s.\n",
                reg.index(), flatIndex, val.print());
    }

    void
    setCCReg(RegIndex reg_idx, RegVal val) override
    {
        int flatIndex = isa->flattenCCIndex(reg_idx);
        assert(flatIndex < ThePpuISA::NumCCRegs);
        DPRINTF(PpuCCRegs, "Setting CC reg %d (%d) to %#x.\n",
                reg_idx, flatIndex, val);
        setCCRegFlat(flatIndex, val);
    }

    ThePpuISA::PCState pcState() const override { return _pcState; }
    void pcState(const ThePpuISA::PCState &val) override { _pcState = val; }

    void
    pcStateNoRecord(const ThePpuISA::PCState &val) override
    {
        _pcState = val;
    }

    Addr instAddr() const override  { return _pcState.instAddr(); }
    Addr nextInstAddr() const override { return _pcState.nextInstAddr(); }
    MicroPC microPC() const override { return _pcState.microPC(); }
    bool readPredicate() const { return predicate; }
    void setPredicate(bool val) { predicate = val; }

    RegVal
    readMiscRegNoEffect(RegIndex misc_reg) const override
    {
        return isa->readMiscRegNoEffect(misc_reg);
    }

    RegVal
    readMiscReg(RegIndex misc_reg) override
    {
        return isa->readMiscReg(misc_reg, this);
    }

    void
    setMiscRegNoEffect(RegIndex misc_reg, RegVal val) override
    {
        return isa->setMiscRegNoEffect(misc_reg, val);
    }

    void
    setMiscRegActuallyNoEffect(int misc_reg, const RegVal &val)
    {
        return isa->setMiscRegNoEffect(misc_reg, val);
    }


    void
    setMiscReg(RegIndex misc_reg, RegVal val) override
    {
        return isa->setMiscReg(misc_reg, val, this);
    }

    RegId
    flattenRegId(const RegId& regId) const override
    {
        return isa->flattenRegId(regId);
    }

    unsigned readStCondFailures() const override { return storeCondFailures; }

    bool
    readMemAccPredicate()
    {
        return memAccPredicate;
    }

    void
    setMemAccPredicate(bool val)
    {
        memAccPredicate = val;
    }

    void
    setStCondFailures(unsigned sc_failures) override
    {
        storeCondFailures = sc_failures;
    }

    Counter
    readFuncExeInst() const override
    {
        return ThreadState::readFuncExeInst();
    }

    void
    syscall(Fault *fault) override
    {
        panic("don't call syscall in Ppu\n");
        // process->syscall(this, fault);
    }

    RegVal readIntRegFlat(RegIndex idx) const override { return intRegs[idx]; }
    void
    setIntRegFlat(RegIndex idx, RegVal val) override
    {
        intRegs[idx] = val;
    }

    RegVal
    readFloatRegFlat(RegIndex idx) const override
    {
        return floatRegs[idx];
    }
    void
    setFloatRegFlat(RegIndex idx, RegVal val) override
    {
        floatRegs[idx] = val;
    }

    const VecRegContainer &
    readVecRegFlat(RegIndex reg) const override
    {
        return vecRegs[reg];
    }

    VecRegContainer &
    getWritableVecRegFlat(RegIndex reg) override
    {
        return vecRegs[reg];
    }

    void
    setVecRegFlat(RegIndex reg, const VecRegContainer &val) override
    {
        vecRegs[reg] = val;
    }

    template <typename T>
    VecLaneT<T, true>
    readVecLaneFlat(RegIndex reg, int lId) const
    {
        return vecRegs[reg].laneView<T>(lId);
    }

    template <typename LD>
    void
    setVecLaneFlat(RegIndex reg, int lId, const LD &val)
    {
        vecRegs[reg].laneView<typename LD::UnderlyingType>(lId) = val;
    }

    const VecElem &
    readVecElemFlat(RegIndex reg, const ElemIndex &elemIndex) const override
    {
        return vecRegs[reg].as<ThePpuISA::VecElem>()[elemIndex];
    }

    void
    setVecElemFlat(RegIndex reg, const ElemIndex &elemIndex,
                   const VecElem &val) override
    {
        vecRegs[reg].as<ThePpuISA::VecElem>()[elemIndex] = val;
    }

    const VecPredRegContainer &
    readVecPredRegFlat(RegIndex reg) const override
    {
        return vecPredRegs[reg];
    }

    VecPredRegContainer &
    getWritableVecPredRegFlat(RegIndex reg) override
    {
        return vecPredRegs[reg];
    }

    void
    setVecPredRegFlat(RegIndex reg, const VecPredRegContainer &val) override
    {
        vecPredRegs[reg] = val;
    }

    RegVal readCCRegFlat(RegIndex idx) const override { return ccRegs[idx]; }
    void setCCRegFlat(RegIndex idx, RegVal val) override { ccRegs[idx] = val; }
};

};


#endif // __PPU_CPU_EXEC_CONTEXT_HH__