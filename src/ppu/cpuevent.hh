/*
 * Copyright (c) 2006 The Regents of The University of Michigan
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
 * Authors: Ali Saidi
 */

#ifndef __PPU_CPUEVENT_HH__
#define __PPU_CPUEVENT_HH__

#include <vector>

#include "sim/eventq.hh"

#if 0
#ifdef BUILD_PPU
namespace PpuISA {
#endif

class PpuThreadContext;
#ifdef BUILD_PPU
};
using namespace PpuISA;
#endif
#endif
class PpuThreadContext;

/**
 * This class creates a global list of events that need a pointer to a
 * thread context. When a switchover takes place the events can be
 * migrated to the new thread context, otherwise you could have a wake
 * timer interrupt go off on a switched out cpu or other unfortunate
 * events. This object MUST be dynamically allocated to avoid it being
 * deleted after a cpu switch happens.
 */
class PpuCpuEvent : public Event
{
  protected:
    /** type of global list of cpu events. */
    typedef std::vector<PpuCpuEvent *> PpuCpuEventList;

    /** Static list of cpu events that is searched every time a cpu switch
     * happens. */
    static PpuCpuEventList cpuEventList;

    /** The thread context that is switched to the new cpus. */
    PpuThreadContext *tc;

  public:
    PpuCpuEvent(PpuThreadContext *_tc, Priority p = Default_Pri)
        : Event(p), tc(_tc)
    { cpuEventList.push_back(this); }

    /** delete the cpu event from the global list. */
    ~PpuCpuEvent();

    /** Update all events switching old tc to new tc.
     * @param oldTc the old thread context we are switching from
     * @param newTc the new thread context we are switching to.
     */
    static void replaceThreadContext(PpuThreadContext *oldTc,
                                     PpuThreadContext *newTc);
    PpuThreadContext* getTC() { return tc; }
};

template <class T, void (T::* F)(PpuThreadContext *tc)>
class PpuCpuEventWrapper : public PpuCpuEvent
{
  private:
    T *object;

  public:
    PpuCpuEventWrapper(T *obj, PpuThreadContext *_tc, Priority p = Default_Pri)
        : PpuCpuEvent(_tc, p), object(obj)
    { }
    void process() { (object->*F)(tc); }
};

#endif // __PPU_CPUEVENT_HH__

