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

#ifndef __DEV_PPU_TIMER_CPU_HH__
#define __DEV_PPU_TIMER_CPU_HH__

#include "dev/io_device.hh"
#include "params/PpuTimerCpu.hh"

class PpuBaseCPU;

class PpuTimerCpu : public BasicPioDevice
{
  public:
    enum {
      Time      = 0x00,
      TimeCmp   = 0x08,
      Size      = 0x10
    };

  private:
    /** cpu the timer belongs to */
    PpuBaseCPU *cpu;

    /** mtimecmp register */
    uint64_t timecmp;

    /** called if time == timecmp */
    void timerAlarm();
    EventFunctionWrapper timerAlarmEvent;

    /** start the timer, if a value is written in timecmp */
    void startTimer(uint64_t val);

  public:
    typedef PpuTimerCpuParams Params;
    const Params *
    params() const
    {
      return dynamic_cast<const Params *>(_params);
    }

    /**
     * The constructor.
     */
    PpuTimerCpu(Params *p);

    /**
     * Overrides pure virtual read method of pio device.
     * Handles a read to the timer device.
     */
    Tick read(PacketPtr pkt) override;

    /**
     * Overrides pure virtual write method of pio device.
     * Handles a write to the timer device.
     */
    Tick write(PacketPtr pkt) override;
};

#endif // __DEV_PPU_TIMER_CPU_HH__