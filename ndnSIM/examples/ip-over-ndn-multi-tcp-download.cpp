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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include "ns3/internet-module.h"
#include "ns3/applications-module.h"
#include "ns3/virtual-net-device.h"

#include "ns3/ndnSIM-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/global-router-interface.h"

#include "ns3/packet.h"
#include "ns3/header.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"
#include "ns3/packet-metadata.h"


#include "ip-over-ndn/ipoc-client.hpp"
#include "ip-over-ndn/gateway-app.hpp"
#include "ip-over-ndn/parse-config.hpp"


/**
 * This scenario simulates a very simple network topology:
 *
 *
 *
 *    (IP-application + Ipoc-Client) ----- (NDN-Router) ------ (Ipoc-Gateway) ----- (IP-Server)
 *
 *
 *
 *
 * Consumer sends interest with Payload object attatched. Router will
 * forward the interest toward Producer.
 * Producer checks incomming interests and finds out the
 * interest carries Payload, thus the Producer extract the Payload.
 *
 * For every received interest, producer replies with a data packet, containing 1024 bytes
 * of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *  NS_LOG=ndn.IpocClient:ndn.GatewayApp ./waf --run "ndn-ip-over-icn"
 */



namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ndn.IpOverNdn");


int
main(int argc, char* argv[])
{
    // Read optional command-line parameters
    //(e.g., enable visualizer with ./waf --run=<> --visualize

    //auto m_config = "src/ndnSIM/examples/ip-over-ndn/config.conf";
    std::string m_config = "";

    CommandLine cmd;
    cmd.AddValue("configName", "config name", m_config);
    cmd.Parse(argc, argv);

  	ParseConfig pc(m_config);
  	pc.setVariables();

    Time::SetResolution (Time::NS);
    LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
    //LogComponentEnable ("BulkSendApplication", LOG_LEVEL_INFO);
    LogComponentEnable ("BulkSendApplication", LOG_LEVEL_INFO);

    std::cout << "Simulation error rate: " << pc.p_simulationErrorRate << std::endl;
    // setting default parameters for PointToPoint links and channels
    Config::SetDefault ("ns3::RateErrorModel::ErrorRate", DoubleValue (pc.p_simulationErrorRate));
    Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));

    Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("40Gbps"));
    Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("5ms"));
    Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("20000"));
    //Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (1400));
    Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1448));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue (500000));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(500000));
    Config::SetDefault ("ns3::TcpL4Protocol::SocketType", StringValue ("ns3::TcpNewReno"));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(16));

    //Enable packet metadata
    Packet::EnablePrinting ();

    // Creating nodes
    NodeContainer nodes;
    nodes.Create(4);

    // Connecting nodes using two links
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    NetDeviceContainer d0d1 = p2p.Install(nodes.Get(0), nodes.Get(1));

    //
    // examples/error-model/simple-error-model.cc
    // https://www.nsnam.org/docs/models/html/error-model.html
    //
    // Error model
    //
    // Create an ErrorModel based on the implementation (constructor)
    // specified by the default TypeId

    ObjectFactory factory;
    factory.SetTypeId ("ns3::RateErrorModel");
    Ptr<ErrorModel> em = factory.Create<ErrorModel> ();
    d0d1.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));

    p2p.SetDeviceAttribute("DataRate", StringValue("40Gbps"));
    NetDeviceContainer d1d2 = p2p.Install(nodes.Get(1), nodes.Get(2));
    NetDeviceContainer d2d3 = p2p.Install(nodes.Get(2), nodes.Get(3));

    NS_LOG_DEBUG("D0D1 : Interface 0 : Address " << d0d1.Get(0)->GetAddress());
    NS_LOG_DEBUG("D0D1 : Interface 1 : Address " << d0d1.Get(1)->GetAddress());
    NS_LOG_DEBUG("D1D2 : Interface 0 : Address " << d1d2.Get(0)->GetAddress());
    NS_LOG_DEBUG("D1D2 : Interface 1 : Address " << d1d2.Get(1)->GetAddress());

    //Install the internet stack
    InternetStackHelper ipHelper;
    ipHelper.Install (nodes.Get(2));
    //ipHelper.SetTcp ("ns3::NscTcpL4Protocol",
    //                 "Library", StringValue ("liblinux2.6.26.so"));

    ipHelper.Install (nodes.Get(0));
    ipHelper.Install (nodes.Get(3));

    // Install NDN stack on nodes 0, 1
    NS_LOG_INFO("Installing NDN Stack");
    ndn::StackHelper ndnHelper;
    ndnHelper.SetDefaultRoutes(true);
    ndnHelper.Install(nodes.Get(0));
    ndnHelper.Install(nodes.Get(1));
    ndnHelper.Install(nodes.Get(2));

    // Install NDN Routes Manually
    ndn::FibHelper::AddRoute(nodes.Get(0), ndn::Name("/ndnSIM"), nodes.Get(1), 1);
    ndn::FibHelper::AddRoute(nodes.Get(1), ndn::Name("/ndnSIM"), nodes.Get(2), 1);

    //Assign IP addresses to other nodes
    NS_LOG_INFO ("Assigning IP Addresses...");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("13.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer i2i3 = ipv4.Assign (d2d3);

    //calculate routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    // Inject global routes from Node 1, including transit network, and recalculate
    Ptr<GlobalRouter> globalRouter2 = nodes.Get(2)->GetObject<GlobalRouter> ();
    globalRouter2->InjectRoute ("11.0.0.0", "255.255.255.0");
    Ipv4GlobalRoutingHelper::RecomputeRoutingTables ();

    // create interface between client application and virtual netdevice on the client side
    Ptr<VirtualNetDevice> tapClt;
    tapClt = CreateObject<VirtualNetDevice> ();
    tapClt->SetAddress (Mac48Address ("11:00:01:02:03:01"));
    tapClt->SetNeedsArp(false);
    nodes.Get(0)->AddDevice (tapClt);
    Ptr<Ipv4> ipv4n0 = nodes.Get(0)->GetObject<Ipv4> ();
    uint32_t i = ipv4n0->AddInterface (tapClt);
    ipv4n0->AddAddress (i, Ipv4InterfaceAddress (Ipv4Address ("11.0.0.12"), Ipv4Mask ("255.255.255.0")));
    Ipv4StaticRoutingHelper helper;
    Ptr<Ipv4StaticRouting> Ipv4stat = helper.GetStaticRouting(ipv4n0);
    Ipv4stat->SetDefaultRoute(Ipv4Address("11.0.0.1"), i, 0);
    ipv4n0->SetUp (i);

    // create interface for the gateway. This is the conection between GW and outside world
    Ptr<VirtualNetDevice> tapGw;
    tapGw = CreateObject<VirtualNetDevice> ();
    tapGw->SetAddress (Mac48Address ("11:00:01:02:03:02"));
    tapGw->SetNeedsArp(false);
    nodes.Get(2)->AddDevice (tapGw);
    Ptr<Ipv4> ipv4n2 = nodes.Get(2)->GetObject<Ipv4> ();
    i = ipv4n2->AddInterface (tapGw);
    ipv4n2->AddAddress (i, Ipv4InterfaceAddress (Ipv4Address ("11.0.0.1"), Ipv4Mask ("255.255.255.0")));
    ipv4n2->SetUp (i);

    Ptr<Node> n0 = nodes.Get (0); // Get pointer to ith node in the server
    Ptr<Ipv4> n0Ipv4 = n0->GetObject<Ipv4> (); // Get Ipv4 instance of the node

    int id = 0;
    float startTime = 2000; //2.0s
    for (const auto& app : pc.p_apps) {
        std::cout << "Application " << id << " setting up ... " << std::endl;

        std::cout << "Configuration:" << std::endl;
        std::cout << "Bytes " << app.bytesList << " intervalList " << app.timeIntervalList << std::endl;
        vector<double> bytes2DurVct;
        istringstream iss(app.bytesList);
        do
        {
            string sub;
            iss >> sub;
            if (sub.size() != 0) {
                double dur = stof(sub)*8/50000000;
                if (dur < 1.0) {
                    bytes2DurVct.push_back(dur + 1);
                }
                else {
                    bytes2DurVct.push_back(dur);
                }
            }
        } while (iss);

        vector<double> intervalVct;
        iss.clear();
        istringstream iss2(app.timeIntervalList);
        do
        {
            string sub;
            iss2 >> sub;
            if (sub.size() != 0) {
                intervalVct.push_back(stof(sub));
            }
        } while (iss2);

        Ptr<DeterministicRandomVariable> onTime = CreateObject<DeterministicRandomVariable> ();
        Ptr<DeterministicRandomVariable> offTime = CreateObject<DeterministicRandomVariable> ();
        std::vector<double> vOnTime;
        std::vector<double> vOffTime;

        //DEBUG messages
        std::cout << "DEBUG OnTime :" << std::endl;
        for (const auto& item : bytes2DurVct) {
            //vOnTime.push_back(item);
            std::cout << item << " ";
        }
        std::cout << std::endl;
        onTime->SetValueArray (&bytes2DurVct[0], bytes2DurVct.size());

        std::cout << "DEBUG OffTime :" << std::endl;
        for (const auto& item : intervalVct) {
            //vOffTime.push_back(item);
            std::cout << item << " ";
        }
        std::cout << std::endl;
        offTime->SetValueArray (&intervalVct[0], intervalVct.size());

        // using timeinterval in each OnOffApplication
        OnOffHelper clientHelper ("ns3::TcpSocketFactory",
                                  Address (InetSocketAddress (Ipv4Address ("11.0.0.12"), 8080 + id)));

        clientHelper.SetAttribute ("MaxBytes", UintegerValue (500000000));
        clientHelper.SetAttribute ("PacketSize", UintegerValue (pc.p_sendSize));
        clientHelper.SetAttribute ("DataRate", DataRateValue (DataRate ("100Mbps")));
        clientHelper.SetAttribute ("OnTime", PointerValue (onTime));
        clientHelper.SetAttribute ("OffTime", PointerValue (offTime));

        ApplicationContainer srvApp = clientHelper.Install (nodes.Get (3));

        //startTime += intervalVct[i];
        srvApp.Start (MilliSeconds (startTime));

        //set the remote address
        PacketSinkHelper sink ("ns3::TcpSocketFactory",
                               Address (InetSocketAddress (Ipv4Address::GetAny (), 8080 + id)));
        ApplicationContainer cltApp = sink.Install (nodes.Get (0));
        cltApp.Start (MilliSeconds(startTime));

        id++;
    }

    // Install NDN app on the client side; encap/decap
    ndn::AppHelper requesterHelper("ns3::ndn::IpocClient");
    requesterHelper.SetAttribute("VirtualNetDevice", (PointerValue)tapClt);
    requesterHelper.SetAttribute("Name", StringValue(pc.p_name));
    requesterHelper.SetAttribute("timer0", UintegerValue(pc.p_timer0));
    requesterHelper.SetAttribute("timer1", UintegerValue(pc.p_timer1));
    requesterHelper.SetAttribute("MaxIDC", UintegerValue(pc.p_MaxIDC));
    requesterHelper.SetAttribute("reseqLen", UintegerValue(pc.p_reseqLen));
    requesterHelper.SetAttribute("waitForGap", UintegerValue(pc.p_waitForGap));

    // start time @1
    ApplicationContainer ipocClt = requesterHelper.Install(nodes.Get(0));
    ipocClt.Start (Seconds(1.0));

    // Install NDN app on the gateway
    ndn::AppHelper producerHelper("ns3::ndn::GatewayApp");
    producerHelper.SetPrefix(pc.p_prefix);
    producerHelper.SetAttribute("PayloadSize", UintegerValue(pc.p_payloadSize));
    producerHelper.SetAttribute("ContentFreshness", TimeValue(Seconds(pc.p_contentFreshness)));
    producerHelper.SetAttribute("CitTableWait", UintegerValue(pc.p_citTableWait));
    producerHelper.SetAttribute("MaxCitEntrySize", UintegerValue(pc.p_maxCitEntrySize));
    producerHelper.SetAttribute("MinCitEntrySize", UintegerValue(pc.p_minCitEntrySize));
    producerHelper.SetAttribute("VirtualNetDevice", (PointerValue)tapGw);
    producerHelper.SetAttribute("waitForGap", UintegerValue(pc.p_waitForGap));
    // start time @1
    ApplicationContainer ipocGw = producerHelper.Install(nodes.Get(2));
    ipocGw.Start (Seconds(1.0));

    std::string simName = "ip-over-ndn-multi-tcp-download-" + std::to_string(pc.p_simulationErrorRate);
    //create traces
    //AsciiTraceHelper ascii;
    //p2p.EnablePcapAll (simName);
    //simName += ".tr";
    //p2p.EnableAsciiAll (ascii.CreateFileStream (simName));

    Simulator::Stop(Seconds(400.0));

    Simulator::Run();
    Simulator::Destroy();

    std::cout << "Finished: " << "Config File " << m_config << std::endl;
    return 0;
}
} // namespace ns3




int
main(int argc, char* argv[])
{
    return ns3::main(argc, argv);
}
