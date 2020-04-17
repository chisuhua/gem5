#include "arch/ppu/locked_mem.hh"

#include <stack>

#include "base/types.hh"

namespace PpuISA
{
    std::unordered_map<int, std::stack<Addr>> locked_addrs;
}
