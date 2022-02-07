// #include "../libcuda/gpu-sim.h"
#include <cassert>

namespace libcuda {
class gpgpu_sim;
}

struct device_id {
  device_id(libcuda::gpgpu_sim *gpu) {
    m_id = 0;
    m_next = nullptr;
    m_gpgpu = gpu;
  }
  struct device_id *next() {
    return m_next;
  }
  int num_devices() const {
    if (m_next == nullptr)
      return 1;
    else
      return 1 + m_next->num_devices();
  }
  struct device_id *get_device(unsigned n) {
    assert(n < (unsigned)num_devices());
    struct device_id *p = this;
    for (unsigned i = 0; i < n; i++) p = p->m_next;
    return p;
  }
  unsigned get_id() const { return m_id; }

  libcuda::gpgpu_sim *get_gpgpu() { return m_gpgpu; }

 protected:
  libcuda::gpgpu_sim *m_gpgpu;
  unsigned m_id;
  struct device_id *m_next;
};


struct CUctx {
  CUctx(device_id *gpu) {
    m_gpu = gpu;
  }
  device_id *get_device() { return m_gpu; }

  device_id *m_gpu;  // selected gpu
};
