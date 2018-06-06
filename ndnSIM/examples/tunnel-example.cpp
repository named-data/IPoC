/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

//TODO
/*

NS_LOG=TunnelExample ./waf --run=tun-example

X 0. Print out the interfaces and IP addresses

X let client send pkts to 11.0.0.2, see if the local module could get the pkt
X 1. tunnel class
X 2. callback function when sending pkts, print out messages
    then send out?

? Do we need to set the Gateway MAC address?
3. callback function when recving pkts, print out messages
    then forward to upper application?

*/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/virtual-net-device.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TunnelExample");
class Tunnel
{
  Ptr<Socket> m_n0Socket;
  Ipv4Address m_n0Address;
  Ptr<VirtualNetDevice> m_n0Tap;


  bool
  N0VirtualSend (Ptr<Packet> packet, const Address& source, const Address& dest, uint16_t protocolNumber)
  {
    //NS_LOG_DEBUG ("Send to " << m_n3Address << ": " << *packet);
    NS_LOG_DEBUG ("Protocol number " << protocolNumber);
    NS_LOG_DEBUG("pkt size " << packet->GetSize ());

    // print the content of my packet on the standard output.
    // no content for now
    packet->Print (std::cout);
    std::cout << std::endl;
    Ipv4Header header;
    uint32_t len = packet->PeekHeader(header);
    NS_LOG_DEBUG("len = " << len);
    NS_LOG_DEBUG("header " << header);

    //NS_LOG_DEBUG("Print Packet " << packet->Print ());
    
    NS_LOG_DEBUG ("Send Callback: Send to " << dest << " : " << *packet);
    //m_n0Socket->SendTo (packet, 0, InetSocketAddress (m_n3Address, 667));
    return true;
  }

  void N0SocketRecv (Ptr<Socket> socket)
  {
    Ptr<Packet> packet = socket->Recv (65535, 0);
    NS_LOG_DEBUG ("Receive Callback: N0SocketRecv: " << *packet);
    //SocketAddressTag socketAddressTag;
    //packet->RemovePacketTag (socketAddressTag);
    //m_n0Tap->Receive (packet, 0x0800, m_n0Tap->GetAddress (), m_n0Tap->GetAddress (), NetDevice::PACKET_HOST);
  }

public:

  Tunnel (Ptr<Node> n0, Ipv4Address n0Addr)
    : m_n0Address (n0Addr)
  {
    m_n0Socket = Socket::CreateSocket (n0, TypeId::LookupByName ("ns3::UdpSocketFactory"));
    m_n0Socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), 667));
    m_n0Socket->SetRecvCallback (MakeCallback (&Tunnel::N0SocketRecv, this));

    // n0 tap device
    m_n0Tap = CreateObject<VirtualNetDevice> ();
    m_n0Tap->SetAddress (Mac48Address ("11:00:01:02:03:01"));
    m_n0Tap->SetSendCallback (MakeCallback (&Tunnel::N0VirtualSend, this));
    m_n0Tap->SetNeedsArp(false);
    n0->AddDevice (m_n0Tap);
    Ptr<Ipv4> ipv4 = n0->GetObject<Ipv4> ();
    uint32_t i = ipv4->AddInterface (m_n0Tap);
    ipv4->AddAddress (i, Ipv4InterfaceAddress (Ipv4Address ("11.0.0.1"), Ipv4Mask ("255.255.255.0")));
    ipv4->SetUp (i);
  }


};



int
main (int argc, char *argv[])
{
  Time::SetResolution (Time::NS);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  //now there is a tunnel with two end points 10.1.1.1 and 10.1.1.2
  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  Tunnel tunnel(nodes.Get(0), nodes.Get(0)->GetObject<Ipv4>()->GetAddress (1, 0).GetLocal());

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (nodes.Get (1));
  std::cout << "Server Address: "<< nodes.Get(1)->GetObject<Ipv4>()->GetAddress(1,0).GetLocal() << std::endl;
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

 // UdpEchoClientHelper echoClient (interfaces.GetAddress (1), 9);
 // std::cout << interfaces.GetAddress (1) << std::endl;
  UdpEchoClientHelper echoClient (Ipv4Address ("11.0.0.2"), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  // create VirtualNetDevice 
  //Ptr<VirtualNetDevice> vd = CreateObject<VirtualNetDevice>();
  //vd->SetAddress (Mac48Address ("01:02:03:04:05:06"));
  //nodes.Get(0)->AddDevice(vd);
  //Ptr<Ipv4> ipv4 = nodes.Get(0)->GetObject<Ipv4>();
  //uint32_t i = ipv4->AddInterface(vd);
  //ipv4->AddAddress(i, Ipv4InterfaceAddress (Ipv4Address ("11.0.0.1"),
  //                Ipv4Mask("255.255.255.0")));
  //ipv4->SetUp(i);

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  std::cout << "Node 0, address at 0, 0: " << nodes.Get(0)->GetObject<Ipv4>()->GetAddress (0, 0).GetLocal () << std::endl;;
  std::cout << "Node 0, address at 1, 0: " << nodes.Get(0)->GetObject<Ipv4>()->GetAddress (1, 0).GetLocal () << std::endl;;
  std::cout << "Node 0, address at 2, 0: " << nodes.Get(0)->GetObject<Ipv4>()->GetAddress (2, 0).GetLocal () << std::endl;;
  std::cout << "Node 1, address at 0, 0: " << nodes.Get(1)->GetObject<Ipv4>()->GetAddress (0, 0).GetLocal () << std::endl;;
  std::cout << "Node 1, address at 1, 0: " << nodes.Get(1)->GetObject<Ipv4>()->GetAddress (1, 0).GetLocal () << std::endl;;

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
