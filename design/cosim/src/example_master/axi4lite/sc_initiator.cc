// #include "base/random.hh"
// #include "traffic_generator.hh"
#include "sc_initiator.hh"
#include "top.hh"

Initiator::Initiator(sc_core::sc_module_name name, Top &top)
  : sc_core::sc_module(name),
    top(top),
    bridge_to_socket("bridge_to_socket"),
    requestInProgress(0),
    peq(this, &Initiator::peq_cb)
{
    socket.register_nb_transport_bw(this, &Initiator::nb_transport_bw);

    bridge_to_socket.bind(top.master_bridge.socket);
    // top.bridge.socket.bind(bridge_to_socket);
    bridge_to_socket.register_nb_transport_fw(this, &Initiator::nb_transport_fw);

    // SC_THREAD(process);
}

/* TLM-2 non-blocking transport method */
tlm::tlm_sync_enum Initiator::nb_transport_fw(tlm::tlm_generic_payload& trans,
                                           tlm::tlm_phase& phase,
                                           sc_time& delay)
{
    auto status = socket->nb_transport_fw(trans, phase, delay);
    return tlm::TLM_ACCEPTED;
}

/*
void
Initiator::process()
{
    auto rnd = Random(time(NULL));

    unsigned const memSize = (1 << 10); // 512 MB

    while (true) {

        wait(sc_core::sc_time((double)rnd.random(1,100), sc_core::SC_NS));

        auto trans = mm.allocate();
        trans->acquire();

        std::string cmdStr;
        if (rnd.random(0,1)) // Generate a write request?
        {
            cmdStr = "write";
            trans->set_command(tlm::TLM_WRITE_COMMAND);
            dataBuffer = rnd.random(0,0xffff);
        } else {
            cmdStr = "read";
            trans->set_command(tlm::TLM_READ_COMMAND);
        }

        trans->set_data_ptr(reinterpret_cast<unsigned char*>(&dataBuffer));
        trans->set_address(rnd.random(0, (int)(memSize-1)));
        trans->set_data_length(4);
        trans->set_streaming_width(4);
        trans->set_byte_enable_ptr(0);
        trans->set_dmi_allowed(0);
        trans->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);

        // honor the BEGIN_REQ/END_REQ exclusion rule
        if (requestInProgress)
            sc_core::wait(endRequestEvent);

        std::stringstream ss;
        ss << "Send " << cmdStr << " request @0x" << std::hex
           << trans->get_address();
        SC_REPORT_INFO("Traffic Generator", ss.str().c_str());

        // send the request
        requestInProgress = trans;
        tlm::tlm_phase phase = tlm::BEGIN_REQ;
        auto delay = sc_core::SC_ZERO_TIME;

        auto status = socket->nb_transport_fw(*trans, phase, delay);

        // Check status
        if (status == tlm::TLM_UPDATED) {
            peq.notify(*trans, phase, delay);
        } else if (status == tlm::TLM_COMPLETED) {
            requestInProgress = 0;
            checkTransaction(*trans);
            SC_REPORT_INFO("Traffic Generator", "request completed");
            trans->release();
        }
    }
}
*/

void
Initiator::peq_cb(tlm::tlm_generic_payload& trans,
                         const tlm::tlm_phase& phase)
{
    if (phase == tlm::END_REQ ||
        (&trans == requestInProgress && phase == tlm::BEGIN_RESP)) {
        // The end of the BEGIN_REQ phase
        requestInProgress = 0;
        endRequestEvent.notify();
    } else if (phase == tlm::BEGIN_REQ || phase == tlm::END_RESP)
        SC_REPORT_FATAL("TLM-2",
                        "Illegal transaction phase received by initiator");

    if (phase == tlm::BEGIN_RESP) {
        checkTransaction(trans);
        SC_REPORT_INFO("Traffic Generator", "received response");

        // Send end response
        tlm::tlm_phase fw_phase = tlm::END_RESP;

        // stress the retry mechanism by deferring the response
        auto delay = sc_core::sc_time(5.0, sc_core::SC_NS);
        socket->nb_transport_fw(trans, fw_phase, delay);
        trans.release();
    }
}

void
Initiator::checkTransaction(tlm::tlm_generic_payload& trans)
{
    if (trans.is_response_error()) {
        std::stringstream ss;
        ss << "Transaction returned with error, response status = %s"
           << trans.get_response_string();
        SC_REPORT_ERROR("TLM-2", ss.str().c_str());
    }
}

tlm::tlm_sync_enum
Initiator::nb_transport_bw(tlm::tlm_generic_payload& trans,
                                  tlm::tlm_phase& phase,
                                  sc_core::sc_time& delay)
{
    trans.acquire();
    peq.notify(trans, phase, delay);
    return tlm::TLM_ACCEPTED;
}
