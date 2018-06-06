/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include "ns3/internet-module.h"
#include "ns3/applications-module.h"

#include "pure-ip-multi-tcp-upload-nsc/parse-config.hpp"

#include <vector>
#include <sstream>
#include <string>
#include <iostream>


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
 *  NS_LOG=ndn.IpocClient:ndn.GatewayApp ./waf --run "pure-ip-single-tcp-upload"
 */


namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ndn.PureIpMultiTcpUpload");

static void
RxDrop (Ptr<const Packet> p)
{
    NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());
}

int
main(int argc, char* argv[])
{
    std::string m_config = "";

    CommandLine cmd;
    cmd.AddValue("configName", "config name", m_config);
    cmd.Parse(argc, argv);

    ParseConfig pc(m_config);
    pc.setVariables();

    Time::SetResolution (Time::PS);
    LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
    //LogComponentEnable ("BulkSendServer", LOG_LEVEL_INFO);
    LogComponentEnable ("FileTransferApplication", LOG_LEVEL_INFO);

    std::cout << "Simulation error rate: " << pc.p_simulationErrorRate << std::endl;
    // setting default parameters for PointToPoint links and channels
    //Config::SetDefault ("ns3::RateErrorModel::ErrorRate", DoubleValue (pc.p_simulationErrorRate));
    Config::SetDefault ("ns3::RateErrorModel::ErrorUnit", StringValue ("ERROR_UNIT_PACKET"));

    // let's use default value
    //Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_congestion_control", StringValue ("cubic"));
    //Config::Set ("/NodeList/*/$ns3::Ns3NscStack<linux2.6.26>/net.ipv4.tcp_congestion_control", StringValue ("NewReno"));

    //Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("40Gbps"));
    //Config::Set ("/NodeList/0/DeviceList/*/$ns3::PointToPointNetDevice/DataRate", StringValue ("40Gbps"));
    //Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("5ms"));
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
    nodes.Create(5);

    // Connecting nodes using two links
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute ("DataRate", StringValue ("40Gbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("0ms"));
    NetDeviceContainer d0d1 = p2p.Install(nodes.Get(0), nodes.Get(1));

    p2p.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("5ms"));
    NetDeviceContainer d1d2 = p2p.Install(nodes.Get(1), nodes.Get(2));

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
    d1d2.Get (1)->SetAttribute ("ReceiveErrorModel", PointerValue (em));
    d1d2.Get (1)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));

    p2p.SetDeviceAttribute("DataRate", StringValue("40Gbps"));
    p2p.SetChannelAttribute ("Delay", StringValue ("5ms"));
    //NetDeviceContainer d1d2 = p2p.Install(nodes.Get(1), nodes.Get(2));
    NetDeviceContainer d2d3 = p2p.Install(nodes.Get(2), nodes.Get(3));
    NetDeviceContainer d3d4 = p2p.Install(nodes.Get(3), nodes.Get(4));

    //Install the internet stack
    InternetStackHelper internetRouters;
    InternetStackHelper internetClients;
    internetRouters.Install (nodes.Get(1));
    internetRouters.Install (nodes.Get(2));
    internetRouters.Install (nodes.Get(3));
    internetClients.SetTcp ("ns3::NscTcpL4Protocol",
                            "Library", StringValue ("liblinux2.6.26.so"));
    internetClients.Install (nodes.Get(0));
    //internetClients.Install (nodes.Get(3));
    internetClients.Install (nodes.Get(4));

    //Assign IP addresses to other nodes
    NS_LOG_INFO ("Assigning IP Addresses...");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase ("11.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer i0i1 = ipv4.Assign (d0d1);
    ipv4.SetBase ("12.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer i1i2 = ipv4.Assign (d1d2);
    ipv4.SetBase ("13.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer i2i3 = ipv4.Assign (d2d3);
    ipv4.SetBase ("14.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer i3i4 = ipv4.Assign (d3d4);

    //calculate routing
    Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

    int id = 0;
    float startTime = 2000; //2.0s
    Ptr<Ipv4> n4Ipv4 = nodes.Get(4)->GetObject<Ipv4> (); // Get Ipv4 instance of the node

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
                                  Address (InetSocketAddress (Ipv4Address (n4Ipv4->GetAddress (1, 0).GetLocal ()), 8080)));

        std::cout << "Max Bytes = " << maxBytes << std::endl;
        clientHelper.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
        clientHelper.SetAttribute ("UseReadingTime", BooleanValue (true));
        clientHelper.SetAttribute ("ReadingTime", PointerValue (intervals));
        clientHelper.SetAttribute ("UseFileTransferDuration", BooleanValue (false));
        clientHelper.SetAttribute ("UseRandomFileSize", BooleanValue (true));
        //sourceFtp.SetAttribute ("RandomFileSize", StringValue ("ns3::LogNormalRandomVariable[Mu=14.8|Sigma=2.0]"));
        clientHelper.SetAttribute ("RandomFileSize", PointerValue (fileSizes));

        ApplicationContainer srvApp = clientHelper.Install (nodes.Get (0));

        //startTime += intervalVct[i];
        srvApp.Start (MilliSeconds (startTime));
        id++;
    }

    //set the remote address
    PacketSinkHelper sink ("ns3::TcpSocketFactory",
                           Address (InetSocketAddress (Ipv4Address::GetAny (), 8080)));
    ApplicationContainer cltApp = sink.Install (nodes.Get (4));
    cltApp.Start (MilliSeconds(startTime));

    std::ostringstream converter;
    //converter << std::setprecision(8) << pc.p_simulationErrorRate;
    converter.precision(7);
    converter << std::fixed << pc.p_simulationErrorRate;
    std::string simName = "pure-ip-multi-tcp-upload-nsc-" + converter.str();
    std::cout << pc.p_simulationErrorRate << " "<< simName << std::endl;

    //create traces
    //AsciiTraceHelper ascii;
    NodeContainer pcapNodes;
    //pcapNodes.Add (nodes.Get(0));
    pcapNodes.Add (nodes.Get(1));
    pcapNodes.Add (nodes.Get(3));
    p2p.EnablePcap (simName, pcapNodes);

    //NetDeviceContainer pcapDev;
    //pcapDev.Add (d2d3.Get (0));
    //p2p.EnablePcap (simName, pcapDev);

    //simName += ".tr";
    //p2p.EnableAsciiAll (ascii.CreateFileStream (simName));

    Simulator::Stop(Seconds(400.0));

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
} // namespace ns3

int
main(int argc, char* argv[])
{
    return ns3::main(argc, argv);
}
