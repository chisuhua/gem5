#ifndef __TRAFFIC_GENERATOR_HH__
#define __TRAFFIC_GENERATOR_HH__

#include <tlm_utils/peq_with_cb_and_phase.h>
#include <tlm_utils/simple_initiator_socket.h>

#include <systemc>
#include <tlm>

#include "sc_mm.hh"

class Initiator : public sc_core::sc_module
{
  private:
    Gem5SystemC::MemoryManager mm;

    tlm::tlm_generic_payload* requestInProgress;

    uint32_t dataBuffer;

    sc_core::sc_event endRequestEvent;

    tlm_utils::peq_with_cb_and_phase<Initiator> peq;

  public:
    tlm_utils::simple_initiator_socket<Initiator> socket;

    Top top;
    tlm_utils::simple_target_socket<Target> bridge_to_socket;

    SC_HAS_PROCESS(Initiator);

    Initiator(sc_core::sc_module_name name);

    void process();

    void peq_cb(tlm::tlm_generic_payload& trans, const tlm::tlm_phase& phase);

    void checkTransaction(tlm::tlm_generic_payload& trans);

    virtual tlm::tlm_sync_enum nb_transport_bw(tlm::tlm_generic_payload& trans,
                                               tlm::tlm_phase& phase,
                                               sc_core::sc_time& delay);
};

#endif
