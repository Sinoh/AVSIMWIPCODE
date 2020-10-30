/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

// #include "ns3/core-module.h"
// #include "ns3/core-module.h"
// #include "ns3/internet-module.h"
// #include "ns3/wifi-module.h"
// #include "ns3/mobility-module.h"
// #include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traci-applications-module.h"
// #include "ns3/network-module.h"
#include "ns3/traci-module.h"
// #include "ns3/wave-module.h"
// #include "ns3/ocb-wifi-mac.h"
// #include "ns3/wifi-80211p-helper.h"
// #include "ns3/wave-mac-helper.h"
// #include "ns3/netanim-module.h"
#include <functional>
#include <stdlib.h>
#include "traci-client.h"


// using namespace ns3;


int
main (int argc, char *argv[])
{

  // connect to sumo via traci
  try
    {
      connect("localhost", 4001);
    }
  catch (std::exception& e)
    {
      NS_FATAL_ERROR("Can not connect to sumo via traci: " << e.what());
    }


  /* ... */

  // Simulator::Run ();
  // Simulator::Destroy ();
  return 0;
}

