#include "arch/ppu/utility.hh"

#include "arch/ppu/faults.hh"
#include "arch/ppu/registers.hh"

namespace PpuISA
{

void
initCPU(ThreadContext *tc, int cpuId)
{
    static Fault reset = std::make_shared<Reset>();
    reset->invoke(tc);
}

bool
isRv32(ThreadContext *tc)
{
    MISA misa = tc->readMiscRegNoEffect(MISCREG_ISA);
    if (misa.mxl32)
        return true;
    return false;
}

}
