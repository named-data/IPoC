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
#include "gateway-app.hpp"
#include "cit.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"

#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4.h"
#include "ns3/ethernet-header.h"
#include "ns3/ipv4-raw-socket-factory.h"
#include "ns3/ipv4-raw-socket-impl.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/udp-l4-protocol.h"

#include <memory>

NS_LOG_COMPONENT_DEFINE("ndn.GatewayApp");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(GatewayApp);

TypeId
GatewayApp::GetTypeId(void)
{
    static TypeId tid =
        TypeId("ns3::ndn::GatewayApp")
        .SetParent<App>()
        .AddConstructor<GatewayApp>()
        .AddAttribute("Prefix", "Prefix, for which producer has the data", StringValue("/"),
                      MakeNameAccessor(&GatewayApp::m_prefix), MakeNameChecker())
        .AddAttribute("PayloadSize", "Virtual payload size for Content packets", UintegerValue(1024),
                      MakeUintegerAccessor(&GatewayApp::m_virtualPayloadSize),
                      MakeUintegerChecker<uint32_t>())
        .AddAttribute("ContentFreshness", "Freshness of data packets, if 0, then unlimited freshness",
                      TimeValue(Seconds(0)), MakeTimeAccessor(&GatewayApp::m_freshness),
                      MakeTimeChecker())
        .AddAttribute("MaxCitEntrySize", "Max size of cit entry size",
                      UintegerValue(2), MakeUintegerAccessor(&GatewayApp::m_maxCitEntrySize),
                      MakeUintegerChecker<uint32_t>())
        .AddAttribute("MinCitEntrySize", "Min size of cit entry size",
                      UintegerValue(1), MakeUintegerAccessor(&GatewayApp::m_minCitEntrySize),
                      MakeUintegerChecker<uint32_t>())
        .AddAttribute("VirtualNetDevice", "Pointer of the virtualNetDevice",
                      PointerValue(),
                      MakePointerAccessor(&GatewayApp::m_vnd),
                      MakePointerChecker<VirtualNetDevice>())
        .AddAttribute("Signature",
                      "Fake signature, 0 valid signature (default), other values application-specific",
                      UintegerValue(0), MakeUintegerAccessor(&GatewayApp::m_signature),
                      MakeUintegerChecker<uint32_t>())
        .AddAttribute("KeyLocator",
                      "Name to be used for key locator.  If root, then key locator is not used",
                      NameValue(), MakeNameAccessor(&GatewayApp::m_keyLocator), MakeNameChecker())
        .AddAttribute("CitTableWait", "CitTableWait", UintegerValue(0),
                      MakeUintegerAccessor(&GatewayApp::m_citTableWait),
                      MakeUintegerChecker<uint32_t>())
        .AddAttribute("waitForGap", "waitForGap", UintegerValue(0),
                      MakeUintegerAccessor(&GatewayApp::m_waitForGap),
                      MakeUintegerChecker<uint32_t>())
 
        ;
    return tid;
}

GatewayApp::GatewayApp()
{
    NS_LOG_FUNCTION_NOARGS();
    m_cit = make_unique<Cit>();
    m_interestsReseq = make_unique<Resequencer<shared_ptr<const Interest>>>();

    //std::cout << "params in GatewayApp constructor():" << std::endl;
    //std::cout << "maxCitEntrySize = " << m_maxCitEntrySize << std::endl;
    //std::cout << "minCitEntrySize = " << m_minCitEntrySize << std::endl;
    //std::cout << "CitTableWait = = " << m_citTableWait << std::endl;
}

// have put these code inside the funtions that needs debug (easier to locate the potential bugs)
void
GatewayApp::Printpacket(Ptr<Packet>& pkt)
{
    NS_LOG_DEBUG("pkt size = " << pkt->GetSize());
    pkt->Print(std::cout);

    //EthernetHeader eHeader;
    //uint32_t eLen = pkt->PeekHeader(eHeader);
    //NS_LOG_DEBUG("pk parsed eHeader Len: " << eLen);
    //NS_LOG_DEBUG("pk parsed eHeader " << eHeader);

    Ipv4Header v4Header;
    uint32_t v4Len = pkt->RemoveHeader(v4Header);

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
    pkt->AddHeader(v4Header);
}

// inherited from Application base class.
void
GatewayApp::StartApplication()
{
    NS_LOG_FUNCTION_NOARGS();
    App::StartApplication();

    //set callback for the on the virtualnetdevice
    m_vnd->SetSendCallback (MakeCallback (&GatewayApp::GetIpPackets, this));

    //add NDN route back to the client
    FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);

    //set up NDN signature on this node
    SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));
    if (m_keyLocator.size() > 0) {
        signatureInfo.setKeyLocator(m_keyLocator);
    }
    gwSignature.setInfo(signatureInfo);
    gwSignature.setValue(::ndn::makeNonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));

    //std::cout << "params in GatewayApp::StartApplication():" << std::endl;
    //std::cout << "maxCitEntrySize = " << m_maxCitEntrySize << std::endl;
    //std::cout << "minCitEntrySize = " << m_minCitEntrySize << std::endl;
    //std::cout << "CitTableWait = = " << m_citTableWait << std::endl;

    m_cit->setPrefix(m_prefix);
    m_cit->setEntryParams(m_maxCitEntrySize, m_minCitEntrySize);

    m_interestsReseq->setLastSeq(0);
    m_interestsReseq->SetWaitForGap(m_waitForGap);
}

bool
GatewayApp::GetIpPackets(Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION (this);
    //this->Printpacket(packet);

    // **** TODO: need to parse the correct addr
    auto entry = m_cit->findOrInsertCitEntry(Name("/11/0/0/12"));
    // 1. len(cit entry) == 0
    //    put pkt in buf
    uint32_t len = entry->sizeOfIntRec();
    //NS_LOG_DEBUG("CitEntry size = " << len);

    SequenceNumber32 curIpSeqNum;

    /* debug messages for TCP packets = BEGIN = */
    Ipv4Header v4Header;
    uint32_t v4Len = packet->RemoveHeader(v4Header);
    if (v4Header.GetProtocol() == TcpL4Protocol::PROT_NUMBER) {
        TcpHeader tcpHeader;
        packet->PeekHeader(tcpHeader);
        NS_LOG_INFO("pk parsed tcpHeader " << tcpHeader);
        NS_LOG_INFO("TCP seq nu = " << tcpHeader.GetSequenceNumber() << " CitEntry size = " << len);
        curIpSeqNum = tcpHeader.GetSequenceNumber();
    }
    packet->AddHeader(v4Header);
    /* debug messages for TCP packets = END = */

    if (len == 0) {
        // no pending Interest @GW
        NS_LOG_DEBUG("CitEntry size = 0, add pkt to buffer, TCP seq nu = " << curIpSeqNum << " , current IP buffer size = " << entry->getIpPktBufSize());
        NS_LOG_DEBUG("Before: IP Packet buffer size = " << entry->getIpPktBufSize());
        entry->pushIpPacket(packet);
        NS_LOG_DEBUG("After: IP Packet buffer size = " << entry->getIpPktBufSize());
    } else {
        NS_LOG_DEBUG("CitEntry size != 0; n = " << entry->getMin());
        // 2. len != 0
        uint8_t idr = 0;
        //    2.1 len < n
        //        idr = 1
        if (len < entry->getMin()) {
            NS_LOG_DEBUG("CitEntry size < n, idr = +1");
            idr = 1;
        }
        //    2.2 len >= n
        //        idr = 0
        // TODO: create content with pkt and putData
        auto record = entry->popInterestRecord();
        Simulator::Remove(record->m_timeoutId);
        NS_LOG_DEBUG("Remove event " << record->m_timeoutId.GetUid());
        //    create content with name, pkt and idr
        auto data = make_shared<Data>(Name(m_prefix).append(entry->getCltAddr()).appendSegment(record->getSeqNumber()));
        //NS_LOG_DEBUG("node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());
        data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
        auto pkts = make_shared<std::vector<Ptr<Packet>>>();
        pkts->push_back(std::move(packet));

        auto ipocPkt = make_shared<IPoCPacket>();
        ipocPkt->setControlBits(idr);
        uint64_t seqNu = entry->getSeqNumber();
        ipocPkt->setSequenceNumber(seqNu);
        NS_LOG_DEBUG("node(" << GetNode()->GetId() << ") Data contains: ipocSN = " << seqNu);
        PutData(pkts, data, ipocPkt);
    }

   return true;
}

size_t
GatewayApp::EstimatePktNumber (shared_ptr<Data>& data, shared_ptr<IPoCPacket>& ipocPkt, shared_ptr<CitEntry>& citEntry)
{
    NS_LOG_FUNCTION (this);
    NS_LOG_DEBUG("IP packet buffer size = " << citEntry->getIpPktBuffer().size());
    auto ipPktBuffer(citEntry->getIpPktBuffer());
    NS_LOG_DEBUG("#IP Packets in IP Packet Buffer " << ipPktBuffer.size());
    size_t cnt = 0;
    IpPacketList pktls;
    uint8_t buf[65535];
    while (!ipPktBuffer.empty()) {
        auto tmpData = make_shared<Data>(*data);
        auto pkt = ipPktBuffer.front();
        ipPktBuffer.pop();
        // test if the added pkt within the MTU
        size_t len = pkt->CopyData(buf, pkt->GetSize());
        Block blk = ::ndn::makeBinaryBlock(::ndn::tlv::IpPacket, buf, len);
        pktls.addIpPacketBlock(blk);
        std::fill( std::begin(buf), std::end(buf), 0 );
       
        NS_LOG_DEBUG("Current #IP Packets in IP Packet list " << pktls.size());

        ipocPkt->setPayload(pktls.wireEncode());
        Block ipocBlk = ipocPkt->wireEncode();

        tmpData->setContent(ipocBlk.value(), ipocBlk.value_size());
        tmpData->setSignature(gwSignature);
        auto resBlk = tmpData->wireEncode();
        NS_LOG_DEBUG("Result block size after adding another IP Packet " << resBlk.size());
        if (resBlk.size() > 8000)
            break;
        cnt++;
    }
    NS_LOG_DEBUG("Estimated #IP packets " << cnt);
    NS_LOG_DEBUG("IP packet buffer size = " << citEntry->getIpPktBuffer().size());

    return cnt;
}

void
GatewayApp::PutData(shared_ptr<std::vector<Ptr<Packet>>>& pkts, shared_ptr<Data>& data, shared_ptr<IPoCPacket>& ipocPkt)
{
    NS_LOG_FUNCTION (this);
    // return NDN data to the client node
    NS_LOG_INFO("Data should contain IP packets # = " << pkts->size());

    /////// ip packet list code for test
    uint8_t buf[65535];
    IpPacketList pktls;
    for (auto pkt = pkts->begin(); pkt != pkts->end(); pkt++) {
        //auto pkt = pkts->front();

        /* debug messages for TCP packets = BEGIN = */
        Ipv4Header v4Header;
        uint32_t v4Len = (*pkt)->RemoveHeader(v4Header);
        if (v4Header.GetProtocol() == TcpL4Protocol::PROT_NUMBER) {
            TcpHeader tcpHeader;
            (*pkt)->PeekHeader(tcpHeader);
            NS_LOG_DEBUG("pk parsed tcpHeader " << tcpHeader);
            NS_LOG_DEBUG("TCP seq nu = " << tcpHeader.GetSequenceNumber());
        }
        (*pkt)->AddHeader(v4Header);
        /* debug messages for TCP packets = END = */

        size_t len = (*pkt)->CopyData(buf, (*pkt)->GetSize());
        
        Block blk = ::ndn::makeBinaryBlock(::ndn::tlv::IpPacket, buf, len);
        pktls.addIpPacketBlock(blk);
        std::fill( std::begin(buf), std::end(buf), 0 );
    }

    if (pktls.size() != 0) {
        ipocPkt->setPayload(pktls.wireEncode());
    }
    Block ipocBlk = ipocPkt->wireEncode();

    data->setContent(ipocBlk.value(), ipocBlk.value_size());
    data->setSignature(gwSignature);
    Block dataBlk = data->wireEncode();

    NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());
    NS_LOG_INFO("Data size = " << dataBlk.size());
    m_transmittedDatas(data, this, m_face);
    //send data to NDN stack 
    m_appLink->onReceiveData(*data);
}

void
GatewayApp::StopApplication()
{
    NS_LOG_FUNCTION_NOARGS();
    App::StopApplication();
}

void
GatewayApp::ResequencerCallback(shared_ptr<std::vector<shared_ptr<const Interest>>> intls)
{
    NS_LOG_FUNCTION (this);
    NS_LOG_DEBUG("Interest list needed to send, size = " << intls->size());

    for (auto i = intls->begin(); i != intls->end(); i++) {
        NS_LOG_DEBUG("Processing Payload (if contains) in Interest " << (*i)->getName());
        if ((*i)->hasPayload()) {
            Block payload = (*i)->getPayload();
            Ptr<Packet> pkt = Create<Packet>(payload.value(), payload.value_size());

            //print packet here if needed
            //this->Printpacket(pkt);

            /* debug messages for TCP packets = BEGIN = */
            Ipv4Header v4Header;
            uint32_t v4Len = pkt->RemoveHeader(v4Header);
            if (v4Header.GetProtocol() == TcpL4Protocol::PROT_NUMBER) {
                TcpHeader tcpHeader;
                pkt->PeekHeader(tcpHeader);
                NS_LOG_INFO("pk parsed tcpHeader " << tcpHeader);
                NS_LOG_DEBUG("TCP seq nu = " << tcpHeader.GetSequenceNumber());
            }
            pkt->AddHeader(v4Header);
            /* debug messages for TCP packets = END = */

            m_vnd->Receive (pkt, 0x0800, m_vnd->GetAddress(), m_vnd->GetAddress(), NetDevice::PACKET_HOST);
        }
    }
}

void
GatewayApp::InterestProcessingLogic (shared_ptr<const Interest>& interest)
{
    NS_LOG_FUNCTION (this);
    // example: /ipoc/01/02/03/04/<seq nu>
    // TODO: must check if the Interest name has the correct format
    Name cltAddr = interest->getName().getSubName(1, 4);
    auto citEntry = m_cit->findOrInsertCitEntry(cltAddr);

    bool empty = citEntry->isIpPktBufEmpty();
    shared_ptr<Data> data = nullptr;

    uint32_t entryLen = citEntry->sizeOfIntRec();
    NS_LOG_DEBUG("CitEntry size  = " << entryLen);

    NS_LOG_DEBUG("start to process Interest " << interest->getName());
    if (empty)
        NS_LOG_DEBUG("CitEntry IP buffer empty = true");
    else
        NS_LOG_DEBUG("CitEntry IP buffer empty = flase");

    //  1 len == 0 and pkts in buf
    //       create content with Idr=1
    if (entryLen == 0 && !empty) { // =0 and is not in buffer
        // get as much IP packets as possible, and put them in a NDN packet
        NS_LOG_DEBUG("CitEntry size = 0 and the pkt is in buffer");

        NS_LOG_DEBUG("creating CO with addr/seq(seq in Interest), idr=1, and pkt");
        data = make_shared<Data>(interest->getName());
        NS_LOG_DEBUG("node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());
        data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

        // get all IP packets from buffer
        //pkts->push_back(std::move(packet));

        auto ipocPkt = make_shared<IPoCPacket>();
        // IDR = +1
        ipocPkt->setControlBits(1);
        NS_LOG_DEBUG("Data contains ControlBits = +1");
        uint64_t seqNu = citEntry->getSeqNumber();
        ipocPkt->setSequenceNumber(seqNu);
        NS_LOG_DEBUG("node(" << GetNode()->GetId() << ") Data contains: ipocSN = " << seqNu);

        size_t pktN = EstimatePktNumber(data, ipocPkt, citEntry);
        auto ipPkts = citEntry->getIpPkts(pktN);

        PutData(ipPkts, data, ipocPkt);

    } else {
        // 2. len(cit(addr)) == N
        //  IDR=-1, create content object with the poped name and idr = -1
        //  push seqNu
        if (entryLen >= citEntry->getMax()) {
            // get as much IP packets as possible, and put them in a NDN packet
            NS_LOG_DEBUG("CitEntry len >=  N(" << citEntry->getMax() << ")");
            auto record = citEntry->popInterestRecord();
            NS_LOG_DEBUG("pop name " << record->getSeqNumber());
            NS_LOG_DEBUG("Removing event " << record->m_timeoutId.GetUid());
            Simulator::Remove(record->m_timeoutId);

            NS_LOG_DEBUG("creating CO with addr/seq(poped), idr=-1"); // not contain pkt
            data = make_shared<Data>(Name(m_prefix).append(citEntry->getCltAddr()).appendSegment(record->getSeqNumber()));
            NS_LOG_DEBUG("node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());
            data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

            auto pkts = make_shared<std::vector<Ptr<Packet>>>();
            NS_LOG_DEBUG("Data contains IP packets # = " << pkts->size());

            auto ipocPkt = make_shared<IPoCPacket>();
            // IDR = -1
            ipocPkt->setControlBits(3); //00000011
            NS_LOG_DEBUG("Data contains ControlBits = -1");
            uint64_t seqNu = citEntry->getSeqNumber();
            ipocPkt->setSequenceNumber(seqNu);
            NS_LOG_DEBUG("node(" << GetNode()->GetId() << ") Data contains: ipocSN = " << seqNu);
            PutData(pkts, data, ipocPkt);
        } 

        //  3 len !=0 or
        //    pkt not in buffer or
        //    len == N 
        //    add the seq in CIT entry

        // TODO: check before convert to number
        uint64_t seqNu = interest->getName().get(-1).toSegment();
        NS_LOG_DEBUG("add seqNu " << seqNu << " into CIT ");
        shared_ptr<InterestRecord> record = make_shared<InterestRecord>(seqNu);
        // TODO: Need to figure out the best timeout for the callback (interestlifetime would cause PIT timeout before CIT)
        record->m_timeoutId = Simulator::Schedule(MilliSeconds(interest->getInterestLifetime().count() - 1000), &GatewayApp::OnCitEntryTimeout, this, interest);
        NS_LOG_DEBUG("added timeout event " << record->m_timeoutId.GetUid() << " call in " << interest->getInterestLifetime().count() - 1000 << " ms");
        citEntry->pushInterestRecord(record);
        
    }
    // TODO: check if the Content pointer is null
    //          null: l != 0 or pkt is not in buffer
    //          non-null: l == N or (l == 0 and pkt in buffer)
    //      Send content out
    //}

    if (!m_active)
        return;
}


void
GatewayApp::OnInterest(shared_ptr<const Interest> interest)
{
    NS_LOG_FUNCTION (this);
    App::OnInterest(interest); // tracing inside
    //handle received Interest from the clients
    NS_LOG_DEBUG("Recvd Interest " << interest->getName());

    // print out Interest size
    Block interestBlk = interest->wireEncode();
    NS_LOG_INFO("Interest size = " << interestBlk.size());

    m_interestsReseq->forwardOrQueue(interest, std::bind(&GatewayApp::ResequencerCallback, this, _1));
    InterestProcessingLogic(interest);
}

void
GatewayApp::OnCitEntryTimeout (shared_ptr<const Interest>& interest)
{
    NS_LOG_FUNCTION (this);
    NS_LOG_DEBUG("Processing timed out Interest " << interest->getName());
    // timeout processing
    // 1. len  < n
    //    idr = 1
    // 2. len >= n
    //    2.1 len > n
    //        idr = -1
    //    2.2 len = n
    //        idr = 0
    //    create content with idr and name

    uint8_t idr = 0;
    Name cltAddr = interest->getName().getSubName(1, 4);
    auto citEntry = m_cit->findOrInsertCitEntry(cltAddr);

    uint32_t entryLen = citEntry->sizeOfIntRec();
    uint32_t n = citEntry->getMin();
    NS_LOG_DEBUG("citEntry size = " << entryLen << " n = " << n);
    // no pending Interest, but since it is triggered by timer, should not be empty
    if (entryLen == 0) {
        NS_LOG_DEBUG("No Interests in CitEntry");
        return;
    }
    if (entryLen < n) {
        idr = 1; //01
        NS_LOG_DEBUG("citEntry size (" << entryLen << ") < n (" << n << ")");
        NS_LOG_DEBUG("Set idr = +1");
    } else if (entryLen > n) {
        idr = 3; //11
        NS_LOG_DEBUG("citEntry size (" << entryLen << ") > n (" << n << ")");
        NS_LOG_DEBUG("Set idr = -1");
    } // len == n: idr = 0;
    // pop the front (should be the first item to time out, if not, think about using list or set)
    auto record = citEntry->popInterestRecord();
    
    // TODO: create object with poped entry and idr
    auto data = make_shared<Data>(Name(m_prefix).append(citEntry->getCltAddr()).appendSegment(record->getSeqNumber()));
    NS_LOG_DEBUG("node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());
    data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
    auto pkts = make_shared<std::vector<Ptr<Packet>>>();

    auto ipocPkt = make_shared<IPoCPacket>();
    ipocPkt->setControlBits(idr);
    uint64_t seqNu = citEntry->getSeqNumber();
    ipocPkt->setSequenceNumber(seqNu);
    NS_LOG_DEBUG("node(" << GetNode()->GetId() << ") Data contains: ipocSN = " << seqNu);
    PutData(pkts, data, ipocPkt);

}


} // namespace ndn
} // namespace ns3
