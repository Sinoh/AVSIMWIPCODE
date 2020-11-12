/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/*
 * Copyright (c) 2018 TU Dresden
 *
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
 * Authors: Patrick Schmager <patrick.schmager@tu-dresden.de>
 *          Sebastian Kuehlmorgen <sebastian.kuehlmorgen@tu-dresden.de>
 */

#include <exception>
#include <algorithm>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iterator>
#include "ns3/core-module.h"


#include "traci-client.h"

namespace ns3
{
  NS_LOG_COMPONENT_DEFINE("TraciClient");

  TypeId
  TraciClient::GetTypeId(void)
  {
    static TypeId tid =
        TypeId("ns3::TraciClient").SetParent<Object>()
    .SetGroupName ("TraciClient")
    .AddAttribute ("AirSimPort",
                  "Port on which SUMO/Traci is listening for connection.",
                  UintegerValue (4001),
                  MakeUintegerAccessor (&TraciClient::m_cntrlPort),
                  MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("AirSimWaitForSocket",
                  "Wait XX sec (=1e6 microsec) until sumo opens socket for traci connection.",
                  TimeValue (ns3::Seconds(8.0)),
                  MakeTimeAccessor (&TraciClient::m_sumoWaitForSocket),
                  MakeTimeChecker ())
    .AddAttribute ("SynchInterval",
                  "Time interval for synchronizing the two simulators.",
                  TimeValue (ns3::Seconds(10.0)),
                  MakeTimeAccessor (&TraciClient::m_synchInterval),
                  MakeTimeChecker ())
    .AddAttribute ("StartTime",
                  "Start time of SUMO simulator; Offset time between ns3 and sumo simulation.",
                  TimeValue (ns3::Seconds(0.0)),
                  MakeTimeAccessor (&TraciClient::m_startTime),
                  MakeTimeChecker ())
    .AddAttribute ("checkTime",
                  "Interval ns3 checks if airSim is finished simulating.",
                  TimeValue (ns3::MilliSeconds(5.0)),
                  MakeTimeAccessor (&TraciClient::m_checkTime_ms),
                  MakeTimeChecker ())
  ;
    return tid;
  }

  TraciClient::TraciClient(void)
  {
    NS_LOG_FUNCTION(this);

    m_sumoSeed = 0;
    m_altitude = 1.5;
    m_cntrlPort = 4001;
    m_sumoGUI = false;
    m_penetrationRate = 1.0;
    m_sumoLogFile = false;
    m_sumoStepLog = false;
    m_sumoWaitForSocket = ns3::Seconds(0);
    m_checkTime_ms = ns3::MilliSeconds(5);
  }

  // destructor!
  TraciClient::~TraciClient(void)
  {
    NS_LOG_FUNCTION(this);
    SumoStop();
  }

  void
  TraciClient::SumoStop()
  {
    NS_LOG_FUNCTION(this);

    try
      {
        this->TraCIAPI::close();
      }
    catch (std::exception& e)
      {
        NS_FATAL_ERROR("Problem while closing traci socket: " << e.what());
      }
  }

  std::string
  TraciClient::GetVehicleId(Ptr<Node> node)
  {
    NS_LOG_FUNCTION(this);

    std::string foundVeh("");

    // search map for corresponding node
    for (std::map<std::string, Ptr<Node> >::iterator it = m_vehicleNodeMap.begin(); it != m_vehicleNodeMap.end(); ++it)
      {
        if (it->second == node)
          {
            foundVeh = it->first;
            break;
          }
      }

    return foundVeh;
  }


  void
  TraciClient::SumoSetup(std::function<Ptr<Node>()> includeNode, std::function<void (Ptr<Node>)> excludeNode)
  {
    NS_LOG_FUNCTION(this);

    m_cntrlPort = GetFreePort(m_cntrlPort); // should be 4001

    m_includeNode = includeNode;
    m_excludeNode = excludeNode;
    

    // wait 1 sec (=1e6 microsec) until sumo opens socket for traci connection
    std::cout << "Sumo: wait for socket: " << m_sumoWaitForSocket.GetSeconds() << "s for port " << m_cntrlPort << std::endl;
    usleep(m_sumoWaitForSocket.GetMicroSeconds());

    // connect to sumo via traci
    try
      {
        this->TraCIAPI::connect("localhost", m_cntrlPort);
      }
    catch (std::exception& e)
      {
        NS_FATAL_ERROR("Can not connect to sumo via traci: " << e.what());
      }

    // start sumo and simulate until the specified time
    std::cout << "starting AirSim and simulate until " << m_startTime.GetSeconds();
    this->TraCIAPI::simulationStep(m_startTime.GetSeconds());


    // synchronise sumo vehicles with ns3 nodes
    SynchroniseVehicleNodeMap();

    std::cout << "Finished synchroniseVehicleNodeMap, now going to updatePositions";

    // get current positions from sumo and uptdate positions
    UpdatePositions();

    // schedule event to command sumo the next simulation step
    Simulator::Schedule(m_synchInterval, &TraciClient::SumoSimulationStep, this);
  }

  // // wait for AirSim to tell us how many cars in simulation
  // TODO: future- make a Car class and add relevant features and return Car
  void
  TraciClient::getCarInitMsg() {
    std::vector<unsigned char>  buffer;
    std::cout << "buffer size is: " << buffer.size();
    tcpip::Storage inMsg;
    int num_cars;
    
    while(buffer.size() == 0){
      buffer = controlSocket->receive();

      if (buffer.size() > 0){
        inMsg.writePacket(&buffer[0], buffer.size());
        
        // next lines just for debug
        std::cout << "\nReceived msg at " << Simulator::Now().GetSeconds() <<  " seconds with msg:\n    ";
        for (unsigned i=0; i<buffer.size(); i++)
          std::cout << buffer.at(i);
        std::cout << "\n";

        num_cars = buffer.at(0) - int('0'); // get int
      }
      else{
        usleep(m_checkTime_ms.GetMicroSeconds());
      }
    }
    // return num_cars; 
  }


  // for string delimiter
  std::vector< std::string> TraciClient::split (std::string s, std::string delimiter){
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector< std::string> res;

    while ((pos_end = s.find (delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
  }

  // connect to car sockets
  void TraciClient::setupCars(){
    for (std::list<tcpip::Socket*>::iterator it = carSockets.begin(); it != carSockets.end(); it++){
        std::cout << "Iterating through the cars\n";
        std::vector<unsigned char>  msg = (*it)->receive();
        std::cout << "msg contains:\n";
        for (unsigned i=0; i<msg.size(); i++)
          std::cout << msg.at(i);
        std::cout << "\n";

        std::string delimiter = ",";
        std::string msgStr(msg.begin(), msg.end());
        std::vector<std::string> v = split (msgStr, delimiter);
        std::string carId;
        int loopNum = 0;
        for (auto data : v){
          if (loopNum == 0){
            // std::cout << data << std::endl;
            carId = data;
            std::cout << "CarId: " << carId << "\n" << std::endl;
          }
          loopNum += 1;
        } 

        // it is not in the map, create a new ns3 node for it
        // create new node by calling the include function
        Ptr<ns3::Node> inNode = m_includeNode();
        // register in the map (link vehicle to node!)
        m_vehicleNodeMap.insert(std::pair<std::string, Ptr<Node>>(carId, inNode));

    }
    // for (int i=0; i < carSockets.size();i++){
    //   std::cout << "for car " << i << ", read init msg and get id, create node and add to map, and add to mobility\n";
      
    // }
    // TraciClient::getCarInitMsg();
    std::cout << "m_vehicleNodeMap size is: " << m_vehicleNodeMap.size() << "\n";
  }

  void
  TraciClient::airSimSetup(std::function<Ptr<Node>()> includeNode, std::function<void (Ptr<Node>)> excludeNode)
  {
    NS_LOG_FUNCTION(this);

    m_cntrlPort = GetFreePort(m_cntrlPort);

    m_includeNode = includeNode;
    m_excludeNode = excludeNode;

  
    TraCIAPI::setUpListeningSocket("localhost", m_cntrlPort);


    std::cout << "Finished connection setup to airSim with:\n    Synctime: " << m_synchInterval.GetSeconds() << "s\n    port " << m_cntrlPort << "\n    numberOfCars: " <<  carSockets.size() <<"\n" << std::endl;

    // TODO: here we want to get all initial positions -> add to map -> then send start Sim command

    setupCars();

    // for (int i=0; i < carSockets.size();i++){
    //   std::cout << "for car " << i << ", read init msg and get id, create node and add to map, and add to mobility\n";
    // }
    


    // this->TraCIAPI::send_simulateCommand(m_startTime.GetSeconds());


    // ns3::Time checkTime = ns3::MilliSeconds(1); // next time to schedule to check if there is a packxet to read 
    // ns3::Time expireTime = Simulator::Now() + m_synchInterval; // end of AirSims simulation time

    // std::cout << "scheduling checkIfReceived at" << checkTime.GetSeconds() << "s and I am at" << Simulator::Now().GetSeconds() << " with expiration of " << expireTime.GetSeconds() <<"\n" ;

    // Simulator::Schedule(checkTime, &TraciClient::checkIfReceived, this, expireTime);
  }

  void
  TraciClient::readBroadcasts(ns3::Time sendBTime) {
    std::cout << "time to read broadcasts!! Here with broadcast Time: "<< sendBTime;

    // Simulator::Schedule(Simulator::Now(), &TraciClient::readBroadcasts, this, Simulator::Now() + m_synchInterval);
  }

  void
  TraciClient::checkIfReceived(ns3::Time expireTime) {
    std::vector<unsigned char>  inMsg;
    ns3::Time checkTime = ns3::MilliSeconds(1) + Simulator::Now();

    // usleep()

    // check if data to read, if there is -> schedule broadcast
    if (Simulator::Now() < expireTime){
      inMsg = controlSocket->receive();

      if (inMsg.size() > 0){
        std::cout << "\nRECEIVED! at " << Simulator::Now().GetSeconds() << "with expiration of: " << expireTime.GetSeconds() << "scheduling next interval at " << checkTime.GetSeconds() << "\n";
        std::cout << "inMsg contains:";
        for (unsigned i=0; i<inMsg.size(); i++)
          std::cout << inMsg.at(i);
        std::cout << "\n";
      }
      Simulator::Schedule(checkTime , &TraciClient::checkIfReceived, this, expireTime);
    }
    else{
      std::cout << "\nTime expired -> send new simulation command" << ns3::Simulator::Now().GetSeconds() << "\n";
      // send new simulation step now
      this->TraCIAPI::send_simulateCommand(Simulator::Now().GetSeconds());
      std::cout << "\nchecking if receive anything scheduling next time at " << ns3::Simulator::Now().GetSeconds() + checkTime.GetSeconds() << " with expiration of: " << m_synchInterval.GetSeconds() + expireTime.GetSeconds() << "\n";
      Simulator::Schedule(checkTime, &TraciClient::checkIfReceived,this, m_synchInterval + ns3::Simulator::Now());
    }
  }

  void
  TraciClient::SumoSimulationStep()
  {
    NS_LOG_FUNCTION(this);

    try
      {
        // get current simulation time
        auto nextTime = Simulator::Now().GetSeconds() + m_synchInterval.GetSeconds() + m_startTime.GetSeconds();

        // command sumo to simulate next time step
        this->TraCIAPI::simulationStep(nextTime);

        // include a ns3 node for every new sumo vehicle and exclude arrived vehicles
        SynchroniseVehicleNodeMap();

        // ask sumo for new vehicle positions and update node positions
        UpdatePositions();

        // schedule next event to simulate next time step in sumo
        Simulator::Schedule(m_synchInterval, &TraciClient::SumoSimulationStep, this);
      }
    catch (std::exception& e)
      {
        NS_FATAL_ERROR("Sumo was closed unexpectedly during simulation: " << e.what());
      }
  }

  

  void
  TraciClient::testSumoSimulationStep()
  {
    NS_LOG_FUNCTION(this);
    std::cout << "in testSumoSimulationStep!";

    try
      {
        auto curr = Simulator::Now().GetSeconds();
        std::cout << "goinginto while!";

        while (curr == 2){
          std::cout << "in testSumoSimulationStep 1!";
          usleep(1000000);
          curr = Simulator::Now().GetSeconds();
        }
        std::cout << "new curr " << curr << "\n\n";

        // get current simulation time
        auto nextTime = Simulator::Now().GetSeconds() + m_synchInterval.GetSeconds() + m_startTime.GetSeconds();

        std::cout << "Telling test clients to simulate next time step at " << nextTime << " because our synch Interval is " << m_synchInterval.GetSeconds() << "and startTime= " <<  m_startTime.GetSeconds();

        std::cout << "\nNow: " << curr << "\n";
        // // command sumo to simulate next time step
        // usleep(10);
        this->TraCIAPI::testSimulationStep(nextTime);

        // command sumo to simulate next time step
        // this->TraCIAPI::testSimulationStep(nextTime);

        // include a ns3 node for every new sumo vehicle and exclude arrived vehicles
        // SynchroniseVehicleNodeMap();

        // ask sumo for new vehicle positions and update node positions
        // UpdatePositions();

        // schedule next event to simulate next time step in sumo
        // Simulator::Schedule(m_synchInterval, &TraciClient::testSumoSimulationStep, this);
      }
    catch (std::exception& e)
      {
        NS_FATAL_ERROR("Sumo was closed unexpectedly during simulation: " << e.what());
      }
  }

  void
  TraciClient::UpdatePositions()
  {
    NS_LOG_FUNCTION(this);

    try
      {
        // iterate over all sumo vehicles in map
        for (std::map<std::string, Ptr<Node> >::iterator it = m_vehicleNodeMap.begin(); it != m_vehicleNodeMap.end(); ++it)
          {
            // get current sumo vehicle from map
            std::string veh(it->first);

            // get vehicle position from sumo
            libsumo::TraCIPosition pos(this->TraCIAPI::vehicle.getPosition(veh));

            // get corresponding ns3 node from map
            Ptr<MobilityModel> mob = m_vehicleNodeMap.at(veh)->GetObject<MobilityModel>();
            // set ns3 node position with user defined altitude
            mob->SetPosition(Vector(pos.x, pos.y, m_altitude));
          }
      }
    catch (std::exception& e)
      {
        NS_FATAL_ERROR("SUMO was closed unexpectedly while asking for vehicle positions: " << e.what());
      }
  }

  void
  TraciClient::GetSumoVehicles(std::vector<std::string>& sumoVehicles)
  {
    NS_LOG_FUNCTION(this);
    std::cout << "in GetSumoVehicles";

    // initialize uniform random distribution for penetration rate
    Ptr<UniformRandomVariable> randVar = CreateObject<UniformRandomVariable>();
    randVar->SetAttribute("Min", DoubleValue(0.0));
    randVar->SetAttribute("Max", DoubleValue(1.0));
    sumoVehicles.clear();

    try
      {
        // ask sumo for all (new) departed vehicles SINCE last simulation step (=one synch interval)
        std::vector<std::string> departedVehicles = this->TraCIAPI::simulation.getDepartedIDList();

        // ask sumo for all (new) arrived vehicles SINCE last simulation step (=one synch interval)
        std::vector<std::string> arrivedVehicles = this->TraCIAPI::simulation.getArrivedIDList();

        // iterate over departed vehicles
        for (std::vector<std::string>::iterator it = departedVehicles.begin(); it != departedVehicles.end(); ++it)
          {
            // get departed vehicle
            std::string veh(*it);

            // search for same vehicle in arrived vehicles
            std::vector<std::string>::iterator pos = std::find(arrivedVehicles.begin(), arrivedVehicles.end(), veh);

            // if vehicle is found in both lists, ignore it; all others are considered as relevant vehicles for simulation
            if (pos != arrivedVehicles.end())
              {
                arrivedVehicles.erase(pos);
              }
            else
              {
                // penetration rate determines number of included nodes
                if (randVar->GetValue() <= m_penetrationRate)
                  {
                    sumoVehicles.push_back(veh);
                  }
              }
          }

        // iterate over arrived vehicles
        for (std::vector<std::string>::iterator it = arrivedVehicles.begin(); it != arrivedVehicles.end(); ++it)
          {
            // get arrived vehicle
            std::string veh(*it);

            // search for arrived vehicle in vehicleNodeMap
            std::map<std::string, Ptr<Node> >::iterator pos = m_vehicleNodeMap.find(veh);

            // if node is in map, exclude it, otherwise is was not simulated in ns3 because of the penetration rate
            if (pos != m_vehicleNodeMap.end())
              {
                sumoVehicles.push_back(veh);
              }
          }
      }
    catch (std::exception& e)
      {
        NS_FATAL_ERROR("SUMO was closed unexpectedly while asking for arrived/departed vehicles: " << e.what());
      }
  }

  void
  TraciClient::SynchroniseVehicleNodeMap()
  {
    NS_LOG_FUNCTION(this);

    try
      {
        // get departed and arrived sumo vehicles since last simulation step
        std::vector<std::string> sumoVehicles;
        GetSumoVehicles(sumoVehicles);

        // iterate over all sumo vehicles with changes; include departed vehicles, exclude arrived vehicles
        for (std::vector<std::string>::iterator it = sumoVehicles.begin(); it != sumoVehicles.end(); ++it)
          {
            // get current vehicle
            std::string veh(*it);

            // search for vehicle in vehicleNodeMap
            std::map<std::string, Ptr<Node> >::iterator pos = m_vehicleNodeMap.find(veh);

            // if it is already in the map, remove it and exclude node
            if (pos != m_vehicleNodeMap.end())
              {
                // get corresponding ns3 node
                Ptr<ns3::Node> exNode = m_vehicleNodeMap.at(veh);

                // call exclude function for this node
                m_excludeNode(exNode);

                // unregister in map
                m_vehicleNodeMap.erase(veh);
              }
            else // if it is not in the map, create a new ns3 node for it
              {
                // create new node by calling the include function
                Ptr<ns3::Node> inNode = m_includeNode();

                // register in the map (link vehicle to node!)
                m_vehicleNodeMap.insert(std::pair<std::string, Ptr<Node>>(veh, inNode));
              }
          }
      }
    catch (std::exception& e)
      {
        NS_FATAL_ERROR("SUMO was closed unexpectedly while updating the vehicle node map: " << e.what());
      }
  }

  void
  TraciClient::testSynchroniseVehicleNodeMap()
  {
    NS_LOG_FUNCTION(this);

    Ptr<ns3::Node> inNode = m_includeNode();

    // TODO: add node to map... 
    // register in the map (link vehicle to node!)
    
    // m_vehicleNodeMap.insert(std::pair<std::string, Ptr<Node>>(veh, inNode));
  }

uint32_t
TraciClient::GetVehicleMapSize()
{
return m_vehicleNodeMap.size();
}

bool
TraciClient::PortFreeCheck (uint32_t portNum)
{
    int socketFd;
    struct sockaddr_in address;

    // Creating socket file descriptor
    if ((socketFd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
      perror("socket failed");
      exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( portNum );

    // Forcefully attaching socket to the specified port
    if (bind(socketFd, (struct sockaddr *)&address, sizeof(address))<0)
    {
      std::cout << "in PortFreeCheck where port not available - bind failed\n";
      // port not available
      return false;
    }
    else
    {
      // port available
      ::close(socketFd); // goto to top empty namespace to avoid conclict with TraCIAPI::close()
      return true;
    }
}

uint32_t
TraciClient::GetFreePort (uint32_t portNum)
{
    uint32_t port = portNum;

    while (!PortFreeCheck(port))
    {
      ++port;
    }

return port;
}

} // namespace ns3

