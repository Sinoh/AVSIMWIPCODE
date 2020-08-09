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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-star.h"
#include "ns3/applications-module.h"

// Default Network Topology
//
//       10.1.1.0
// n0 -------------- n1
//    point-to-point
//
 
using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("StarP2PDemo");

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);
  
  Time::SetResolution (Time::NS); // ns time resolution (default value) - can only be changed once IOT conserve memory
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO); // enable logging at the INFO level for echo clients and servers
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO); // ^this will result in the application printing messages when packets are sent/recieved

  // Setup Attributes for P2P Star
  int nSpokes = 3;
  PointToPointHelper p2pHelper;
  p2pHelper.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  p2pHelper.SetChannelAttribute ("Delay", StringValue ("2ms"));

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  InternetStackHelper stack;
 
  // Create P2P Star and Assign Attributes
  PointToPointStarHelper p2pStarHelper (nSpokes, p2pHelper);
  p2pStarHelper.InstallStack (stack);
  p2pStarHelper.AssignIpv4Addresses (address);
  
  // Schedule Echo Sequence
  UdpEchoServerHelper echoServer (9); // arbitrary port

  ApplicationContainer serverApps = echoServer.Install (p2pStarHelper.GetHub ());
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  for (int i = 0; i < nSpokes; i++)
    {
      
      UdpEchoClientHelper echoClient (p2pStarHelper.GetHubIpv4Address (i), 9);
      echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
      echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
      echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

      ApplicationContainer clientApps = echoClient.Install (p2pStarHelper.GetSpokeNode (i));
      clientApps.Start (Seconds (1.0 + i));
      clientApps.Stop (Seconds (10.0));
    }

  // Run Simulation
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}