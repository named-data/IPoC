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

#include "cit-entry.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.CitEntry");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(CitEntry);

TypeId
CitEntry::GetTypeId()
{
  static TypeId tid = TypeId("ns3::ndn::CitEntry").SetGroupName("Ndn").SetParent<Object>();
  return tid;
}

uint32_t
CitEntry::GetId() const
{
  return m_id;
}

CitEntry::CitEntry(const Name& cltAddr, uint32_t max, uint32_t min)
    : m_maxLen(max)
    , m_minLen(min)
    , m_curSeqNum(0)
    , m_cltAddr(cltAddr)
{
}

uint32_t
CitEntry::sizeOfIntRec()
{
    return m_interestRecords.size();
}

uint32_t
CitEntry::getMax()
{
    return m_maxLen;
}

uint32_t
CitEntry::getMin()
{
    return m_minLen;
}

shared_ptr<std::vector<Ptr<Packet>>>
CitEntry::getIpPkts(const size_t n)
{
    NS_LOG_DEBUG("Get " << n << " IP packets");
    auto pkts = make_shared<std::vector<Ptr<Packet>>>();
    size_t cnt = 0;
    while (!m_ipPktBuffer.empty() && cnt < n) {
        auto pkt = m_ipPktBuffer.front();
        m_ipPktBuffer.pop();
        pkts->push_back(pkt);
        cnt++;
    }
    return pkts;
}

const Name
CitEntry::getCltAddr()
{
    return m_cltAddr;
}


uint32_t
CitEntry::getSeqNumber()
{
    uint32_t res = m_curSeqNum;
    // TODO: mod if overflow
    m_curSeqNum++;
    return res;
}

void
CitEntry::pushInterestRecord(shared_ptr<InterestRecord>& record)
{
    m_interestRecords.push(record);
}

void
CitEntry::pushIpPacket(Ptr<Packet>& ipPkt)
{
    m_ipPktBuffer.push(ipPkt);
}

shared_ptr<InterestRecord>
CitEntry::popInterestRecord()
{
    auto res = m_interestRecords.front();
    m_interestRecords.pop();
    return res;
}


} // namespace ndn
} // namespace ns3
