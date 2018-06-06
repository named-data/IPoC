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
#include "ipoc-client.hpp"

#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/log.h"

#include "ns3/ndnSIM/ndn-cxx/interest.hpp"
#include "ns3/ndnSIM/ndn-cxx/encoding/block-helpers.hpp"

#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/ethernet-header.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/udp-l4-protocol.h"

#include <bitset>

NS_LOG_COMPONENT_DEFINE("ndn.IpocClient");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(IpocClient);

TypeId
IpocClient::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ndn::IpocClient")
                        .SetParent<App>()
                        .AddConstructor<IpocClient>()
                        .AddAttribute("Name", "Name of data to request",
                                      StringValue("/data/name"),
                                      MakeNameAccessor(&IpocClient::m_name),
                                      MakeNameChecker())
                        .AddAttribute("VirtualNetDevice", "Pointer of the virtualNetDevice",
                                      PointerValue(),
                                      MakePointerAccessor(&IpocClient::m_vnd),
                                      MakePointerChecker<VirtualNetDevice>())
                        .AddAttribute("timer0", "Timer 0", UintegerValue(0),
                                      MakeUintegerAccessor(&IpocClient::m_timer0),
                                      MakeUintegerChecker<uint32_t>())
                        .AddAttribute("timer1", "Timer 1", UintegerValue(0),
                                      MakeUintegerAccessor(&IpocClient::m_timer1),
                                      MakeUintegerChecker<uint32_t>())
                        .AddAttribute("MaxIDC", "maxIDC", UintegerValue(0),
                                      MakeUintegerAccessor(&IpocClient::m_idc),
                                      MakeUintegerChecker<uint32_t>())
                        .AddAttribute("reseqLen", "reseqLen", UintegerValue(0),
                                      MakeUintegerAccessor(&IpocClient::m_reseqLen),
                                      MakeUintegerChecker<uint32_t>())
                        .AddAttribute("waitForGap", "waitForGap", UintegerValue(0),
                                      MakeUintegerAccessor(&IpocClient::m_waitForGap),
                                      MakeUintegerChecker<uint32_t>())
 
                        ;
    return tid;
}

IpocClient::IpocClient()
    : m_interestByIpPktCnt(0)
    , m_interestByTimeoutCnt(0)
    , m_dataPktRecvdCnt(0)
    , m_ipPktRecvdCnt(0)
    , m_ipPktSentCnt(0)
{
    // setting up things
    m_dataReseq = make_shared<Resequencer<shared_ptr<const Data>>>();
	
    m_interestSegmentNum = 0;
    m_curIdc = m_idc;
    //m_dataReseq->SetCbCapacity(m_reseqLen);
    // NS_LOG_DEBUG( "Re len " << m_dataReseq->GetCbSize() << "max " << m_dataReseq->GetCbCapacity());
}

void
IpocClient::Printpacket(Ptr<Packet>& pkt)
{
    NS_LOG_DEBUG("pkt size = " << pkt->GetSize());
    pkt->Print(std::cout);

    //EthernetHeader eHeader;
    //uint32_t eLen = pkt->PeekHeader(eHeader);
    //NS_LOG_DEBUG("pk parsed eHeader Len: " << eLen);
    //NS_LOG_DEBUG("pk parsed eHeader " << eHeader);

    Ipv4Header v4Header;
    uint32_t v4Len = pkt->PeekHeader(v4Header);
    NS_LOG_DEBUG("pk parsed v4Header Len: " << v4Len);
    NS_LOG_DEBUG("pk parsed v4Header " << v4Header);

    if (v4Header.GetProtocol() == UdpL4Protocol::PROT_NUMBER) {
        UdpHeader udpHeader;
        uint32_t udpLen = pkt->PeekHeader(udpHeader);
        NS_LOG_DEBUG("pk parsed UdpHeader  Len: " << udpLen);
        NS_LOG_DEBUG("pk parsed UdpHeader  " << udpHeader);
    }

    if (v4Header.GetProtocol() == TcpL4Protocol::PROT_NUMBER) {
        TcpHeader tcpHeader;
        uint32_t tcpLen = pkt->PeekHeader(tcpHeader);
        NS_LOG_DEBUG("pk parsed tcpHeader Len: " << tcpLen);
        NS_LOG_DEBUG("pk parsed tcpHeader " << tcpHeader);
        NS_LOG_DEBUG("pk parsed tcpHeader src port " << tcpHeader.GetSourcePort ());
    }
}

void IpocClient::ProcessTimer0 ()
{
    NS_LOG_FUNCTION (this);

    NS_LOG_DEBUG("Timer 0 " << m_timer0 << " curIdc " << m_curIdc);
    if (m_curIdc > 0) {
        this->SendInterest();
    }
    m_t0SchID = Simulator::Schedule(MicroSeconds(m_timer0), &IpocClient::ProcessTimer0, this);
}

void IpocClient::ProcessTimer1 ()
{
    NS_LOG_FUNCTION (this);

    NS_LOG_DEBUG( "Timer 1 " <<  m_timer1);
    this->SendInterest();
    m_t1SchID = Simulator::Schedule(MicroSeconds(m_timer1), &IpocClient::ProcessTimer0, this);
}



void IpocClient::ScheduleTimers() {
    NS_LOG_FUNCTION (this);

    m_t0SchID = Simulator::Schedule(MicroSeconds(m_timer0), &IpocClient::ProcessTimer0, this);
    m_t1SchID = Simulator::Schedule(MicroSeconds(m_timer1), &IpocClient::ProcessTimer1, this);
    // NS_LOG_DEBUG( "m_t0SchID " << m_t0SchID.GetUid());
    // NS_LOG_DEBUG( "m_t1SchID " << m_t1SchID.GetUid());
}

void
IpocClient::StartApplication()
{
    App::StartApplication();

    m_dataReseq->setLastSeq(0);
    NS_LOG_DEBUG("Wait for gap " << m_waitForGap << " ms");
    m_dataReseq->SetWaitForGap(m_waitForGap);


    // the callback must be here, after the IpocClient app is created
    m_vnd->SetSendCallback (MakeCallback (&IpocClient::GetIpPackets, this));
    this->ScheduleTimers();
}


void
IpocClient::StopApplication()
{
    // do cleanup
    App::StopApplication();
    m_face->close();
}


void IpocClient::AdjustIDC(int32_t adjustVal) {
    NS_LOG_FUNCTION (this);
    NS_LOG_DEBUG("adjustVal = " << adjustVal);

    int32_t minIdc = -m_idc;
    int32_t maxIdc = m_idc;

    int32_t new_idc = m_curIdc + adjustVal;

    if (new_idc < minIdc) {
        m_curIdc = minIdc;
    }
    else if (new_idc > maxIdc) {
        m_curIdc = maxIdc;
    }

    else {
        m_curIdc = new_idc;
    }
    NS_LOG_DEBUG("Max IDC =  " << maxIdc << " Min IDC " << minIdc << " Current IDC " << m_curIdc << " Adjust value " << adjustVal);
}

bool
IpocClient::GetIpPackets(Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION (this);
    m_ipPktRecvdCnt++;
    NS_LOG_DEBUG("Upstream ipPktCnt = " << m_ipPktRecvdCnt);
    NS_LOG_INFO("IP pkt size =  " << packet->GetSize());

    /* debug messages for TCP packets = BEGIN = */
    Ipv4Header v4Header;
    uint32_t v4Len = packet->RemoveHeader(v4Header);
    if (v4Header.GetProtocol() == TcpL4Protocol::PROT_NUMBER) {
        TcpHeader tcpHeader;
        packet->PeekHeader(tcpHeader);
        NS_LOG_INFO("pk parsed tcpHeader " << tcpHeader);
        NS_LOG_DEBUG("TCP seq nu = " << tcpHeader.GetSequenceNumber());
    }
    packet->AddHeader(v4Header);
    /* debug messages for TCP packets = END = */

    //send out the NDN interest
    SendInterest(packet);
    return true;
}



void
IpocClient::SendInterest(void) {
    NS_LOG_FUNCTION (this);

    auto interest = make_shared<Interest>(Name(m_name).appendSegment(m_interestSegmentNum));

    //TODO: make this configurable
    m_interestSegmentNum++;
    interest->setInterestLifetime(time::seconds(4));

    NS_LOG_DEBUG("Sending an Timeout Interest for "<< *interest );

    Block interestBlk = interest->wireEncode();
    NS_LOG_INFO("Sent timeout Interest size = "<< interestBlk.size() );

    m_interestByTimeoutCnt++;
    m_transmittedInterests(interest, this, m_face);
    m_appLink->onReceiveInterest(*interest);

    //cancel T0 and t1 timer event
    Simulator::Remove(m_t0SchID);
    Simulator::Remove(m_t1SchID);

    //reschedule them for later
    this->ScheduleTimers();

    //reduce IDC
    this->AdjustIDC(-1);
}



void
IpocClient::SendInterest(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION (this);
    // TODO: need to start from #0?

    auto interest = make_shared<Interest>(Name(m_name).appendSegment(m_interestSegmentNum));
    NS_LOG_DEBUG( "Sending an Interest for "<< *interest << " interest seq = " << m_interestSegmentNum);
    m_interestSegmentNum++;

    //TODO: make this configurable
    interest->setInterestLifetime(time::seconds(4));

    // use real data pkt
    NS_LOG_DEBUG("Orignal packet length " << packet->GetSize() );
    uint8_t buf[65535];
    size_t len = packet->CopyData(buf, packet->GetSize());
    NS_LOG_DEBUG("Copied packet data, size = " << len );

    Block blk = ::ndn::makeBinaryBlock(::ndn::tlv::IpPacket, buf, len);
    Packet pkt(blk.value(), blk.value_size());

    interest->setPayload(blk);
    NS_LOG_DEBUG("Sending an Interest for "<< *interest );

    Block interestBlk = interest->wireEncode();
    NS_LOG_INFO("Sent Interest size = "<< interestBlk.size() );

    m_interestByIpPktCnt++;
    m_transmittedInterests(interest, this, m_face);
    m_appLink->onReceiveInterest(*interest);

    //cancel T0 and t1 timer event
    Simulator::Remove(m_t0SchID);
    Simulator::Remove(m_t1SchID);

    //reschedule them for later
    this->ScheduleTimers();

    //reduce IDC
    this->AdjustIDC(-1);
}

void
IpocClient::ResequencerCallback(shared_ptr<std::vector<shared_ptr<const Data>>> pktls) {
    NS_LOG_FUNCTION (this);
    for (auto pkt = pktls->begin(); pkt != pktls->end(); pkt++) {
        IPoCPacket ipocPkt((*pkt)->getContent());

        if (ipocPkt.getPayload().value_size() == 0)
            continue;;

        IpPacketList pktls(ipocPkt.getPayload());

        NS_LOG_DEBUG( "Sending #IP pkts = " << pktls.size());

        for (auto i = pktls.begin(); i != pktls.end(); i++) {
            NS_LOG_INFO("IP pkt value size = " << i->value_size());
            Ptr<Packet> pkt = Create<Packet>(i->value(), i->value_size());
            Ipv4Header v4Header;
            size_t len = pkt->RemoveHeader(v4Header);

            /* debug messages for TCP packets = BEGIN = */
            if (v4Header.GetProtocol() == TcpL4Protocol::PROT_NUMBER) {
                TcpHeader tcpHeader;
                pkt->PeekHeader(tcpHeader);
                NS_LOG_INFO("pk parsed tcpHeader " << tcpHeader);
                NS_LOG_DEBUG("TCP seq nu = " << tcpHeader.GetSequenceNumber());
            }
            pkt->AddHeader(v4Header);
            /* debug messages for TCP packets = END = */

            m_vnd->Receive (pkt, 0x0800, m_vnd->GetAddress(), m_vnd->GetAddress(), NetDevice::PACKET_HOST);
            m_ipPktSentCnt++;
        }
    }
}

void
IpocClient::UpdateParams(shared_ptr<const Data>& data) {
    NS_LOG_FUNCTION (this);

    IPoCPacket ipocPkt(data->getContent());
    NS_LOG_DEBUG( "IPoC Data SequenceNumber = " << ipocPkt.getSequenceNumber());

    uint8_t idr = ipocPkt.getControlBits();

    if (idr == 0)
        this->AdjustIDC(0);
    else if (idr == 1)
        this->AdjustIDC(1);
    else if (idr == 3)
        this->AdjustIDC(-1);

    if (ipocPkt.getPayload().value_size() == 0)
        return;

    /* debug messages for TCP packets = BEGIN = */
    IpPacketList pktls(ipocPkt.getPayload());

    for (auto i = pktls.begin(); i != pktls.end(); i++) {
        Ptr<Packet> pkt = Create<Packet>(i->value(), i->value_size());
        Ipv4Header v4Header;
        size_t len = pkt->RemoveHeader(v4Header);

        if (v4Header.GetProtocol() == TcpL4Protocol::PROT_NUMBER) {
            TcpHeader tcpHeader;
            pkt->PeekHeader(tcpHeader);
            NS_LOG_DEBUG("pk parsed tcpHeader " << tcpHeader);
        }
        pkt->AddHeader(v4Header);
    }
    /* debug messages for TCP packets = END = */
}

uint32_t IpocClient::GetSequenceNumber(std::shared_ptr<const ndn::Data>& data) {
    NS_LOG_FUNCTION (this);
    IPoCPacket ipocPkt(data->getContent());
    auto thisSeq = ipocPkt.getSequenceNumber();
    NS_LOG_DEBUG( "SequenceNumber = " << thisSeq);
    return(thisSeq);
}

void
IpocClient::OnData(std::shared_ptr<const ndn::Data> data)
{
    NS_LOG_FUNCTION (this);
    m_dataPktRecvdCnt++;
    NS_LOG_INFO("Data name : " << data->getName());
    NS_LOG_DEBUG( "Name: " << m_name <<  "IDC = " << m_idc << " Cur IDC " << m_curIdc << " Reseq len" <<  m_reseqLen);
    NS_LOG_DEBUG( " IDC before " << m_curIdc);
    this->AdjustIDC(1);
    NS_LOG_DEBUG( " IDC after " << m_curIdc);

    NS_LOG_DEBUG("m_interestByIpPktCnt = " << m_interestByIpPktCnt
                 << " m_interestByTimeoutCnt = " << m_interestByTimeoutCnt
                 << " m_dataPktRecvdCnt = " << m_dataPktRecvdCnt
                 << " m_ipPktRecvdCnt = " << m_ipPktRecvdCnt
                 << " m_ipPktSentCnt = " << m_ipPktSentCnt);

    // using resequencer
    m_dataReseq->forwardOrQueue(data, std::bind(&IpocClient::ResequencerCallback, this, _1));
    this->UpdateParams(data);
}

} // namespace ndn
} // namespace ns3
