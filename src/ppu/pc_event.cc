/*
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
 * Authors: Nathan Binkert
 *          Steve Reinhardt
 */

#include "ppu/pc_event.hh"

#include <algorithm>
#include <string>
#include <utility>

#include "base/debug.hh"
#include "base/trace.hh"
#include "debug/PpuPCEvent.hh"
#include "sim/core.hh"
#include "ppu_sim/system.hh"

using namespace std;

PpuPCEventQueue::PpuPCEventQueue()
{}

PpuPCEventQueue::~PpuPCEventQueue()
{}

bool
PpuPCEventQueue::remove(PpuPCEvent *event)
{
    int removed = 0;
    range_t range = equal_range(event);
    iterator i = range.first;
    while (i != range.second && i != pcMap.end()) {
        if (*i == event) {
            DPRINTF(PpuPCEvent, "PC based event removed at %#x: %s\n",
                    event->pc(), event->descr());
            i = pcMap.erase(i);
            ++removed;
        } else {
            i++;
        }
    }

    return removed > 0;
}

bool
PpuPCEventQueue::schedule(PpuPCEvent *event)
{
    pcMap.push_back(event);
    sort(pcMap.begin(), pcMap.end(), MapCompare());

    DPRINTF(PpuPCEvent, "PC based event scheduled for %#x: %s\n",
            event->pc(), event->descr());

    return true;
}

bool
PpuPCEventQueue::doService(Addr pc, PpuThreadContext *tc)
{
    // Using the raw PC address will fail to break on Alpha PALcode addresses,
    // but that is a rare use case.
    int serviced = 0;
    range_t range = equal_range(pc);
    for (iterator i = range.first; i != range.second; ++i) {
        DPRINTF(PpuPCEvent, "PC based event serviced at %#x: %s\n",
                (*i)->pc(), (*i)->descr());

        (*i)->process(tc);
        ++serviced;
    }

    return serviced > 0;
}

void
PpuPCEventQueue::dump() const
{
    const_iterator i = pcMap.begin();
    const_iterator e = pcMap.end();

    for (; i != e; ++i)
        cprintf("%d: event at %#x: %s\n", curTick(), (*i)->pc(),
                (*i)->descr());
}

PpuPCEventQueue::range_t
PpuPCEventQueue::equal_range(Addr pc)
{
    return std::equal_range(pcMap.begin(), pcMap.end(), pc, MapCompare());
}

BreakPpuPCEvent::BreakPpuPCEvent(PpuPCEventScope *s, const std::string &desc, Addr addr,
                           bool del)
    : PpuPCEvent(s, desc, addr), remove(del)
{
}

void
BreakPpuPCEvent::process(PpuThreadContext *tc)
{
    StringWrap name("break_event");
    DPRINTFN("break event %s triggered\n", descr());
    Debug::breakpoint();
    if (remove)
        delete this;
}

PanicPpuPCEvent::PanicPpuPCEvent(PpuPCEventScope *s, const std::string &desc, Addr pc)
    : PpuPCEvent(s, desc, pc)
{
}

void
PanicPpuPCEvent::process(PpuThreadContext *tc)
{
    StringWrap name("panic_event");
    panic(descr());
}
