/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
*
* Copyright (c) 2017 Cable Television Laboratories, Inc.
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation;
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
* Authors: Susmit Shannigrahi <susmit@colostate.edu>
           Chengyu Fan <chengyu.fan@colostate.edu>
           Greg White <g.white@cablelabs.com>
*
*/

#ifndef CLIENT_INTEREST_TABLE_ENTRY_HPP
#define CLIENT_INTEREST_TABLE_ENTRY_HPP

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "interest-record.hpp"
#include "ns3/packet.h"
#include "ns3/object.h"
#include <queue>

namespace ns3 {

namespace ndn {

class CitEntry  : public Object
{
public:
    /**
     * \brief Interface ID
     *
     * \return interface ID
     */
    static TypeId
    GetTypeId();

    /**
     * @brief Get numeric ID of the node (internally assigned)
     */
    uint32_t
    GetId() const;

    CitEntry(const Name& cltAddr, uint32_t max, uint32_t min);

    //public:
    /** \brief unsatisfy timer
     *
     *  This timer is used in forwarding pipelines to delete the entry
     *  when it expires without being satisfied.
     *  It fires when the last InterestLifetime among in-records expires.
     *
     *  Either this or the straggler timer should be set at all times,
     *  except when this entry is being processed in a pipeline.
     */
    //scheduler::EventId m_unsatisfyTimer;

    uint32_t
    sizeOfIntRec();

    uint32_t
    getMax();

    uint32_t
    getMin();

    // get top n IP packets
    shared_ptr<std::vector<Ptr<Packet>>>
    getIpPkts(const size_t n);

    shared_ptr<InterestRecord>
    popInterestRecord();

    void
    pushInterestRecord(shared_ptr<InterestRecord>& record);

    void
    pushIpPacket(Ptr<Packet>& ipPkt);

    const Name
    getCltAddr();

    uint32_t
    getSeqNumber();

    bool
    isIpPktBufEmpty()
    {
        return m_ipPktBuffer.size() == 0;
    }

    size_t
    getIpPktBufSize()
    {
        return m_ipPktBuffer.size();
    }

    std::queue<Ptr<Packet>>&
    getIpPktBuffer()
    {
        return m_ipPktBuffer;
    }

private:
    uint32_t m_id;
    std::queue<shared_ptr<InterestRecord>> m_interestRecords;
    std::queue<Ptr<Packet>> m_ipPktBuffer;
    uint32_t m_maxLen;
    uint32_t m_minLen;
    uint32_t m_curSeqNum;
    const Name m_cltAddr;
};

} // namespace ndn
} // namespace ns3

#endif // CLIENT_INTEREST_TABLE_ENTRY_HPP
