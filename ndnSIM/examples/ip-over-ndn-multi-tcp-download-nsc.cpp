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


#include "ip-over-ndn-multi-tcp-download-nsc/ipoc-client.hpp"
#include "ip-over-ndn-multi-tcp-download-nsc/gateway-app.hpp"
#include "ip-over-ndn-multi-tcp-download-nsc/parse-config.hpp"


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

NS_LOG_COMPONENT_DEFINE("ndn.IpOverNdnMultiFileDownload");

// unfortunately, does not work for NSC socket ...
//static void
//CwndTracer (uint32_t oldval, uint32_t newval)
//{
//   // NS_LOG_INFO ("Moving cwnd from " << oldval << " to " << newval);
//}

static void
RxDrop (Ptr<const Packet> p)
{
    NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
}

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
    LogComponentEnable ("FileTransferApplication", LOG_LEVEL_INFO);

    std::cout << "Simulation error rate: " << pc.p_simulationErrorRate << std::endl;
    // setting default parameters for PointToPoint links and channels
    //Config::SetDefault ("ns3::RateErrorModel::ErrorRate", DoubleValue (pc.p_simulationErrorRate));
    Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));

    // let's use default value
    //Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_congestion_control", StringValue ("cubic"));
    //Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_congestion_control", StringValue ("NewReno"));

    Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("40Gbps"));
    Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("5ms"));
    Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("20000"));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue (1400));
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

    //ObjectFactory factory;
    //factory.SetTypeId ("ns3::RateErrorModel");
    //Ptr<ErrorModel> em = factory.Create<ErrorModel> ();
    //d0d1.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    DoubleValue rate (pc.p_simulationErrorRate);
    Ptr<RateErrorModel> em = 
        CreateObjectWithAttributes<RateErrorModel> ("RanVar", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"), "ErrorRate", rate);
    d0d1.Get (0)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    d0d1.Get (0)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));

    p2p.SetDeviceAttribute("DataRate", StringValue("40Gbps"));
    NetDeviceContainer d1d2 = p2p.Install(nodes.Get(1), nodes.Get(2));
    NetDeviceContainer d2d3 = p2p.Install(nodes.Get(2), nodes.Get(3));

    NS_LOG_DEBUG("D0D1 : Interface 0 : Address " << d0d1.Get(0)->GetAddress());
    NS_LOG_DEBUG("D0D1 : Interface 1 : Address " << d0d1.Get(1)->GetAddress());
    NS_LOG_DEBUG("D1D2 : Interface 0 : Address " << d1d2.Get(0)->GetAddress());
    NS_LOG_DEBUG("D1D2 : Interface 1 : Address " << d1d2.Get(1)->GetAddress());

    //Install the internet stack
    InternetStackHelper internetRouters;
    InternetStackHelper internetClients;
    internetRouters.Install (nodes.Get(2));
    internetClients.SetTcp ("ns3::NscTcpL4Protocol",
                            "Library", StringValue ("liblinux2.6.26.so"));
    internetClients.Install (nodes.Get(0));
    internetClients.Install (nodes.Get(3));

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

    int id = 0;
    float startTime = 2000; //2.0s
    Ptr<Ipv4> n0Ipv4 = nodes.Get(0)->GetObject<Ipv4> (); // Get Ipv4 instance of the node

    for (const auto& app : pc.p_apps) {
        std::cout << "Application " << id << " setting up ... " << std::endl;

        std::cout << "Configuration:" << std::endl;
        std::cout << "Bytes " << app.bytesList << " intervalList " << app.timeIntervalList << std::endl;
        std::vector<double> bytesVct;
        std::istringstream iss(app.bytesList);
        do
        {
            std::string sub;
            iss >> sub;
            if (sub.size() != 0) {
                bytesVct.push_back(stof(sub));
            }
        } while (iss);

        std::vector<double> intervalVct;
        //iss.clear();
        std::istringstream iss2(app.timeIntervalList);
        do
        {
            std::string sub;
            iss2 >> sub;
            if (sub.size() != 0) {
                intervalVct.push_back(stof(sub));
            }
        } while (iss2);

        Ptr<DeterministicRandomVariable> fileSizes = CreateObject<DeterministicRandomVariable> ();
        Ptr<DeterministicRandomVariable> intervals = CreateObject<DeterministicRandomVariable> ();
        std::vector<double> vOnTime;
        std::vector<double> vOffTime;

        //DEBUG messages
        std::cout << "DEBUG File sizes:" << std::endl;
        int maxBytes = 0;
        for (const auto& item : bytesVct) {
            std::cout << item << " ";
            maxBytes += item;
        }
        std::cout << std::endl;
        fileSizes->SetValueArray (&bytesVct[0], bytesVct.size());

        std::cout << "DEBUG OffTime :" << std::endl;
        for (const auto& item : intervalVct) {
            std::cout << item << " ";
        }
        std::cout << std::endl;
        intervals->SetValueArray (&intervalVct[0], intervalVct.size());

        // using timeinterval in each FileTransferApplication
        FileTransferHelper clientHelper ("ns3::TcpSocketFactory",
                                  Address (InetSocketAddress (Ipv4Address ("11.0.0.12"), 8080)));

        std::cout << "Max Bytes = " << maxBytes << std::endl;
        clientHelper.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
        clientHelper.SetAttribute ("UseReadingTime", BooleanValue (true));
        clientHelper.SetAttribute ("ReadingTime", PointerValue (intervals));
        clientHelper.SetAttribute ("UseFileTransferDuration", BooleanValue (false));
        clientHelper.SetAttribute ("UseRandomFileSize", BooleanValue (true));
        clientHelper.SetAttribute ("RandomFileSize", PointerValue (fileSizes));

        ApplicationContainer srvApp = clientHelper.Install (nodes.Get (3));

        //startTime += intervalVct[i];
        srvApp.Start (MilliSeconds (startTime));
        id++;
    }

    //set the remote address
    PacketSinkHelper sink ("ns3::TcpSocketFactory",
                           Address (InetSocketAddress (Ipv4Address::GetAny (), 8080)));
    ApplicationContainer cltApp = sink.Install (nodes.Get (0));
    cltApp.Start (MilliSeconds(startTime));

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

    // Trace changes to the congestion window, but does not work for NSC ...
    //Config::Set ("/NodeList/3/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_sack", StringValue ("0"));
    // Ptr<NscTcpL4Protocol> n3Nsc = nodes.Get(3)->GetObject<NscTcpL4Protocol> (); // Get Ipv4 instance of the node
    //Config::ConnectWithoutContext ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/SocketList/*/CongestionWindow", MakeCallback (&CwndTracer));

    std::ostringstream converter;
    //converter << std::setprecision(8) << pc.p_simulationErrorRate;
    converter.precision(7);
    converter << std::fixed << pc.p_simulationErrorRate;
    std::string simName = "ip-over-ndn-multi-tcp-download-nsc-" + converter.str();
    std::cout << pc.p_simulationErrorRate << " "<< simName << std::endl;

    //create traces
    //AsciiTraceHelper ascii;
    //p2p.EnablePcapAll (simName);

    NetDeviceContainer pcapDev;
    pcapDev.Add (d2d3.Get (0));
    p2p.EnablePcap (simName, pcapDev);

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
