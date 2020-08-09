/*
 * Copyright (c) 2019 ARM Limited
 * All rights reserved.
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
 * Copyright (c) 1999-2008 Mark D. Hill and David A. Wood
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

/*
 * Unordered buffer of messages that can be inserted such
 * that they can be dequeued after a given delta time has expired.
 */

#ifndef __MEM_RUBY_NETWORK_MESSAGEBUFFER_HH__
#define __MEM_RUBY_NETWORK_MESSAGEBUFFER_HH__

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "base/trace.hh"
#include "debug/RubyQueue.hh"
#include "mem/packet.hh"
#include "mem/port.hh"
#include "mem/ruby/common/Address.hh"
#include "mem/ruby/common/Consumer.hh"
#include "mem/ruby/network/dummy_port.hh"
#include "mem/ruby/slicc_interface/Message.hh"
#include "params/MessageBuffer.hh"
#include "sim/sim_object.hh"

class MessageBuffer : public SimObject
{
  private:
    /**
     * Port on the CPU-side that receives requests.
     * Mostly just forwards requests to the owner.
     * Part of a vector of ports. One for each CPU port (e.g., data, inst)
     */
    class InPort : public SlavePort
    {
      private:
        /// The object that owns this object (SimpleMemobj)
        MessageBuffer *owner;

        /// True if the port needs to send a retry req.
        bool needRetry;

        /// If we tried to send a packet and it was blocked, store it here
        PacketPtr blockedPacket;

      public:
        /**
         * Constructor. Just calls the superclass constructor.
         */
        InPort(const std::string& name, MessageBuffer *owner) :
            SlavePort(name, owner), owner(owner), needRetry(false),
            blockedPacket(nullptr)
        { }

        /**
         * Send a packet across this port. This is called by the owner and
         * all of the flow control is hanled in this function.
         *
         * @param packet to send.
         */
        void sendPacket(PacketPtr pkt);

        /**
         * Get a list of the non-overlapping address ranges the owner is
         * responsible for. All slave ports must override this function
         * and return a populated list with at least one item.
         *
         * @return a list of ranges responded to
         */
        AddrRangeList getAddrRanges() const override;

        /**
         * Send a retry to the peer port only if it is needed. This is called
         * from the SimpleMemobj whenever it is unblocked.
         */
        void trySendRetry();
      protected:
        /**
         * Receive an atomic request packet from the master port.
         * No need to implement in this simple memobj.
         */
        Tick recvAtomic(PacketPtr pkt) override
        { panic("recvAtomic unimpl."); }

        /**
         * Receive a functional request packet from the master port.
         * Performs a "debug" access updating/reading the data in place.
         *
         * @param packet the requestor sent.
         */
        void recvFunctional(PacketPtr pkt) override;

        /**
         * Receive a timing request from the master port.
         *
         * @param the packet that the requestor sent
         * @return whether this object can consume the packet. If false, we
         *         will call sendRetry() when we can try to receive this
         *         request again.
         */
        bool recvTimingReq(PacketPtr pkt) override;

        /**
         * Called by the master port if sendTimingResp was called on this
         * slave port (causing recvTimingResp to be called on the master
         * port) and was unsuccesful.
         */
        void recvRespRetry() override;
    };

    /**
     * Port on the memory-side that receives responses.
     * Mostly just forwards requests to the owner
     */
    class OutPort : public MasterPort
    {
      private:
        /// The object that owns this object (SimpleMemobj)
        MessageBuffer *owner;

        /// If we tried to send a packet and it was blocked, store it here
        PacketPtr blockedPacket;
      public:
        /**
         * Constructor. Just calls the superclass constructor.
         */
        OutPort(const std::string& name, MessageBuffer *owner) :
            MasterPort(name, owner), owner(owner), blockedPacket(nullptr)
        { }

        /**
         * Send a packet across this port. This is called by the owner and
         * all of the flow control is hanled in this function.
         *
         * @param packet to send.
         */
        void sendPacket(PacketPtr pkt);

      protected:
        /**
         * Receive a timing response from the slave port.
         */
        bool recvTimingResp(PacketPtr pkt) override;

        /**
         * Called by the slave port if sendTimingReq was called on this
         * master port (causing recvTimingReq to be called on the slave
         * port) and was unsuccesful.
         */
        void recvReqRetry() override;

        /**
         * Called to receive an address range change from the peer slave
         * port. The default implementation ignores the change and does
         * nothing. Override this function in a derived class if the owner
         * needs to be aware of the address ranges, e.g. in an
         * interconnect component like a bus.
         */
        void recvRangeChange() override;
    };

    /**
     * Handle the request from the CPU side
     *
     * @param requesting packet
     * @return true if we can handle the request this cycle, false if the
     *         requestor needs to retry later
     */
    bool handleEnqueueRequest(PacketPtr pkt);

    /**
     * Handle the respone from the out_port side
     *
     * @param responding packet
     * @return true if we can handle the response this cycle, false if the
     *         responder needs to retry later
     */
    bool handleEnqueueResponse(PacketPtr pkt);

    /**
     * Handle a packet functionally. Update the data on a write and get the
     * data on a read.
     *
     * @param packet to functionally handle
     */
    void handleFunctional(PacketPtr pkt);

    /**
     * Return the address ranges this memobj is responsible for. Just use the
     * same as the next upper level of the hierarchy.
     *
     * @return the address ranges this memobj is responsible for
     */
    AddrRangeList getAddrRanges() const;

    /**
     * Tell the CPU side to ask for our memory ranges.
     */
    void sendRangeChange();

    // port for AXI intercept
    OutPort m_out_port;
    InPort m_in_port;

    /// True if this is currently blocked waiting for a response.
    bool m_enqueue_blocked;
    bool blocked;

  public:
    typedef MessageBufferParams Params;
    MessageBuffer(const Params *p);

    void reanalyzeMessages(Addr addr, Tick current_time);
    void reanalyzeAllMessages(Tick current_time);
    void stallMessage(Addr addr, Tick current_time);

    // TRUE if head of queue timestamp <= SystemTime
    bool isReady(Tick current_time) const;

    void
    delayHead(Tick current_time, Tick delta)
    {
        MsgPtr m = m_prio_heap.front();
        std::pop_heap(m_prio_heap.begin(), m_prio_heap.end(),
                      std::greater<MsgPtr>());
        m_prio_heap.pop_back();
        enqueue(m, current_time, delta);
    }

    bool areNSlotsAvailable(unsigned int n, Tick curTime);
    int getPriority() { return m_priority_rank; }
    void setPriority(int rank) { m_priority_rank = rank; }
    void setConsumer(Consumer* consumer)
    {
        DPRINTF(RubyQueue, "Setting consumer: %s\n", *consumer);
        if (m_consumer != NULL) {
            fatal("Trying to connect %s to MessageBuffer %s. \
                  \n%s already connected. Check the cntrl_id's.\n",
                  *consumer, *this, *m_consumer);
        }
        m_consumer = consumer;
    }

    Consumer* getConsumer() { return m_consumer; }

    bool getOrdered() { return m_strict_fifo; }

    //! Function for extracting the message at the head of the
    //! message queue.  The function assumes that the queue is nonempty.
    const Message* peek() const;

    const MsgPtr &peekMsgPtr() const { return m_prio_heap.front(); }

    void enqueue(MsgPtr message, Tick curTime, Tick delta);

    //! Updates the delay cycles of the message at the head of the queue,
    //! removes it from the queue and returns its total delay.
    Tick dequeue(Tick current_time, bool decrement_messages = true);

    void registerDequeueCallback(std::function<void()> callback);
    void unregisterDequeueCallback();

    void recycle(Tick current_time, Tick recycle_latency);
    bool isEmpty() const { return m_prio_heap.size() == 0; }
    bool isStallMapEmpty() { return m_stall_msg_map.size() == 0; }
    unsigned int getStallMapSize() { return m_stall_msg_map.size(); }

    unsigned int getSize(Tick curTime);

    void clear();
    void print(std::ostream& out) const;
    void clearStats() { m_not_avail_count = 0; m_msg_counter = 0; }

    void setIncomingLink(int link_id) { m_input_link_id = link_id; }
    void setVnet(int net) { m_vnet_id = net; }

    Port &
    getPort(const std::string &if_name, PortID idx=InvalidPortID) override;
/*
    Port &
    getPort(const std::string &, PortID idx=InvalidPortID) override
    {
        return RubyDummyPort::instance();
    }
*/
    void regStats() override;

    // Function for figuring out if any of the messages in the buffer need
    // to be updated with the data from the packet.
    // Return value indicates the number of messages that were updated.
    uint32_t functionalWrite(Packet *pkt)
    {
        return functionalAccess(pkt, false);
    }

    // Function for figuring if message in the buffer has valid data for
    // the packet.
    // Returns true only if a message was found with valid data and the
    // read was performed.
    bool functionalRead(Packet *pkt)
    {
        return functionalAccess(pkt, true) == 1;
    }

  private:
    void reanalyzeList(std::list<MsgPtr> &, Tick);

    uint32_t functionalAccess(Packet *pkt, bool is_read);

  private:
    // Data Members (m_ prefix)
    //! Consumer to signal a wakeup(), can be NULL
    Consumer* m_consumer;
    std::vector<MsgPtr> m_prio_heap;

    std::function<void()> m_dequeue_callback;

    // use a std::map for the stalled messages as this container is
    // sorted and ensures a well-defined iteration order
    typedef std::map<Addr, std::list<MsgPtr> > StallMsgMapType;

    /**
     * A map from line addresses to lists of stalled messages for that line.
     * If this buffer allows the receiver to stall messages, on a stall
     * request, the stalled message is removed from the m_prio_heap and placed
     * in the m_stall_msg_map. Messages are held there until the receiver
     * requests they be reanalyzed, at which point they are moved back to
     * m_prio_heap.
     *
     * NOTE: The stall map holds messages in the order in which they were
     * initially received, and when a line is unblocked, the messages are
     * moved back to the m_prio_heap in the same order. This prevents starving
     * older requests with younger ones.
     */
    StallMsgMapType m_stall_msg_map;

    /**
     * Current size of the stall map.
     * Track the number of messages held in stall map lists. This is used to
     * ensure that if the buffer is finite-sized, it blocks further requests
     * when the m_prio_heap and m_stall_msg_map contain m_max_size messages.
     */
    int m_stall_map_size;

    /**
     * The maximum capacity. For finite-sized buffers, m_max_size stores a
     * number greater than 0 to indicate the maximum allowed number of messages
     * in the buffer at any time. To get infinitely-sized buffers, set buffer
     * size: m_max_size = 0
     */
    const unsigned int m_max_size;

    Tick m_time_last_time_size_checked;
    unsigned int m_size_last_time_size_checked;

    // variables used so enqueues appear to happen immediately, while
    // pop happen the next cycle
    Tick m_time_last_time_enqueue;
    Tick m_time_last_time_pop;
    Tick m_last_arrival_time;

    unsigned int m_size_at_cycle_start;
    unsigned int m_stalled_at_cycle_start;
    unsigned int m_msgs_this_cycle;

    Stats::Scalar m_not_avail_count;  // count the # of times I didn't have N
                                      // slots available
    uint64_t m_msg_counter;
    int m_priority_rank;
    const bool m_strict_fifo;
    const bool m_randomization;

    int m_input_link_id;
    int m_vnet_id;

    Stats::Average m_buf_msgs;
    Stats::Average m_stall_time;
    Stats::Scalar m_stall_count;
    Stats::Formula m_occupancy;
};

Tick random_time();

inline std::ostream&
operator<<(std::ostream& out, const MessageBuffer& obj)
{
    obj.print(out);
    out << std::flush;
    return out;
}

#endif //__MEM_RUBY_NETWORK_MESSAGEBUFFER_HH__
