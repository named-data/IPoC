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

#ifndef NDN_IPOC_CLIENT_H
#define NDN_IPOC_CLIENT_H

#include "resequencer.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/ndnSIM/apps/ndn-app.hpp"

#include "ns3/virtual-net-device.h"

#include "ns3/timer.h"

#include "ns3/simulator.h"

#include "ns3/event-id.h"

namespace ns3 {
namespace ndn {

class IpocClient : public App
{
public:
    static TypeId
    GetTypeId(void);

    IpocClient();

protected:
    virtual void
    StartApplication();

    virtual void
    StopApplication();

    void
    Printpacket(Ptr<Packet>& pkt);

private:
    void
    ScheduleTimers(void);

    void
    AdjustIDC(int32_t adjValue);

    void
    ProcessTimer0(void);

    void
    ProcessTimer1(void);

    void
    SendInterest(void);

    void
    SendInterest(Ptr<Packet> packet);

    virtual void
    OnData(std::shared_ptr<const ndn::Data> data);

    void ReadDataQueue(void);

    bool
    GetIpPackets(Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);

    uint32_t GetSequenceNumber(std::shared_ptr<const ndn::Data>& data);

    void
    ResequencerCallback(shared_ptr<std::vector<shared_ptr<const Data>>> pktls);

    // update IDC, etc. with current Data
    void
    UpdateParams(shared_ptr<const Data>& data);

private:
    Name m_name;
    Ptr<VirtualNetDevice> m_vnd;
    uint32_t m_timer0;
    uint32_t m_timer1;
    int32_t m_idc;
    int32_t m_curIdc;
    uint32_t m_reseqLen;
	uint32_t m_interestSegmentNum;
	uint32_t m_waitForGap;
	uint32_t m_interestByIpPktCnt; // Interests sent out
	uint32_t m_interestByTimeoutCnt; // Interests sent out
    uint32_t m_dataPktRecvdCnt; // data from GW
    uint32_t m_ipPktRecvdCnt; // recvd from virtual net device
    uint32_t m_ipPktSentCnt; // send to virtual net device


    ns3::EventId m_t0SchID;
    ns3::EventId m_t1SchID;
    std::shared_ptr<Resequencer<shared_ptr<const Data>>> m_dataReseq;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_IPOC_CLIENT_H
