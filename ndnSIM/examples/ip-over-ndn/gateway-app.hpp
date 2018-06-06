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

#ifndef NDN_GW_PRODUCER_H
#define NDN_GW_PRODUCER_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/ndnSIM/apps/ndn-app.hpp"

#include "ns3/nstime.h"
#include "ns3/ptr.h"
#include "ns3/socket.h"
#include "ns3/virtual-net-device.h"

#include "cit.hpp"
#include "resequencer.hpp"

#include <memory>
#include <vector>


namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief A simple Interest-sink applia simple Interest-sink application
 *
 * A simple Interest-sink applia simple Interest-sink application,
 * which replying every incoming Interest with Data packet with a specified
 * size and name. Additionally, it extracts the Payload carried by Interest.
 */
class GatewayApp : public App {
public:
    static TypeId
    GetTypeId(void);

    GatewayApp();

    // inherited from NdnApp
    virtual void
    OnInterest(shared_ptr<const Interest> interest);

protected:
    // inherited from Application base class.
    virtual void
    StartApplication(); // Called at time specified by Start

    virtual void
    StopApplication(); // Called at time specified by Stop

    void
    Printpacket(Ptr<Packet>& pkt);

    bool
    GetIpPackets(Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber);

    void
    PutData (shared_ptr<std::vector<Ptr<Packet>>>& pkts, shared_ptr<Data>& data, shared_ptr<IPoCPacket>& ipocPkt);

    void
    OnCitEntryTimeout (shared_ptr<const Interest>& interest);

    size_t
    EstimatePktNumber (shared_ptr<Data>& data, shared_ptr<IPoCPacket>& ipocPkt, shared_ptr<CitEntry>& citEntry);

protected: // resequencer
    void
    ResequencerCallback(shared_ptr<std::vector<shared_ptr<const Interest>>> pktls);

    void
    InterestProcessingLogic(shared_ptr<const Interest>& interest);

private:
    Name m_prefix;
    uint32_t m_virtualPayloadSize;
    Time m_freshness;
    uint32_t m_maxCitEntrySize;
    uint32_t m_minCitEntrySize;

    Signature gwSignature;
    uint32_t m_signature;
    Name m_keyLocator;
    Ptr<VirtualNetDevice> m_vnd;
    std::unique_ptr<Cit> m_cit;
	uint32_t m_citTableWait;
	uint32_t m_waitForGap;

    std::unique_ptr<Resequencer<shared_ptr<const Interest>>> m_interestsReseq;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_PRODUCER_H
