#include "ns3/lte-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/config-store.h"
#include "ns3/data-rate.h"
#include <cfloat>
#include <sstream>

using namespace ns3;

std::map<int, Ptr<Socket>> source_map = {};
std::map<int, Ptr<Socket>> sink_map = {};

class loc_header : public Header 
{
	public:
    loc_header();
     //virtual ~loc_header();
    void SetLocation(const Vector location);
    const Vector GetLocation() const;
    static TypeId GetTypeId (void);
    TypeId GetInstanceTypeId () const;
    virtual uint32_t GetSerializedSize (void) const;
    virtual void Serialize (Buffer::Iterator start) const;
    virtual uint32_t Deserialize (Buffer::Iterator start); 
    virtual void Print (std::ostream &os) const;
    const uint32_t GetSenderId() const;
    void SetSenderId(const uint32_t Id);
    void SetMsgId (const uint32_t MsgId);
    const uint32_t GetMsgId() const;
			
	private:
		Vector m_position;
    uint32_t SenderId;
    uint32_t MID; //MID is message ID
};

NS_OBJECT_ENSURE_REGISTERED (loc_header);

loc_header::loc_header(){}

TypeId loc_header::GetInstanceTypeId () const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void loc_header::SetLocation(const Vector location)
{
	m_position = location;
}

const Vector loc_header::GetLocation() const // const Vector & GetPosition () const; the meaning of & here?
{
	return m_position;
}

TypeId loc_header::GetTypeId(void)
{
	static TypeId tid = TypeId ("ns3::loc_Header").SetParent<Header>().AddConstructor<loc_header>();
	return tid;
}

uint32_t loc_header::GetSerializedSize (void) const
{
	return (3*sizeof(double)+2*sizeof(uint32_t)); // x and y coordinates both are doubles, IDs are uint32_t
}

void loc_header::Serialize (Buffer::Iterator start) const // from Adeel cam header
{
	//NS_LOG_FUNCTION (this << &start);
	//Buffer::Iterator i = start;
	start.WriteHtonU64 (ceil(m_position.x*1000)); // doubles are 8 bytes
	start.WriteHtonU64 (ceil(m_position.y*1000));
	start.WriteHtonU64 (ceil(m_position.z*1000));
  start.WriteHtonU32 (SenderId);
  start.WriteHtonU32(MID);
}

uint32_t loc_header::Deserialize (Buffer::Iterator start)   // from Adeel cam header
{
	m_position.x = start.ReadNtohU64()/1000; // doubles are 8 bytes
	m_position.y = start.ReadNtohU64()/1000;
	m_position.z = start.ReadNtohU64()/1000;
  SenderId = start.ReadNtohU32();
  MID = start.ReadNtohU32();
  return GetSerializedSize ();
}

void loc_header::Print(std::ostream &os) const 
{
  os << "Packet sent by Node "<<SenderId<<" from " << "( " << m_position.x << ", " << m_position.y << ", " << m_position.z << " )" << std::endl;
}

void loc_header::SetSenderId(const uint32_t Id)
{
    SenderId = Id;
}


const uint32_t loc_header::GetSenderId() const
{
    return SenderId;
}

void loc_header::SetMsgId (const uint32_t MsgId)
{
    MID = MsgId;
}

const uint32_t loc_header::GetMsgId() const
{
    return MID;
}

uint32_t MsgCount = 0; // will be used as MsgId to ID the packets from the tx side
uint32_t pktCount = 1;

int32_t total_transmitted_packets = 0; //won't gve the totl packets transmitted, will give the number of times this function was called = nodes*packets (200*3)
static void sendPacket (Ptr<Socket> socket, uint32_t pktSize)
{

  std::cout << "node " << socket->GetNode()->GetId()<<" sending packet\n";
  total_transmitted_packets++;
  Ptr<Packet> p = Create<Packet> (pktSize-(12+24+8)); //12 bytes seqts, 24 bytes loc_header and 4+4 bytes IDs (MsgId and SenderId)
  loc_header new_header;
  Ptr<ConstantVelocityMobilityModel> mob3 = socket->GetNode()->GetObject<ConstantVelocityMobilityModel>(); //https://stackoverflow.com/questions/57290850/how-to-get-and-print-mobile-nodes-position-in-aodv-ns3
  double x_old = mob3->GetPosition().x; 
  double y_old = mob3->GetPosition().y; 
  double z_old = mob3->GetPosition().z;

  std::cout << "sending loc: x- " << x_old << "  y- "<<  y_old << "  z- " << z_old << "\n";
  Vector3D vect(x_old, y_old, z_old);

  new_header.SetLocation(vect);
  new_header.SetSenderId(socket->GetNode()->GetId());
  // std::cout<<"Sender is "<<socket->GetNode()->GetId()<<" and packet is generated from "<<x_old<<", "<<y_old<<", "<<z_old<<", "<<std::endl;
  new_header.SetMsgId(++MsgCount);
  p->AddHeader(new_header);
  SeqTsHeader seqTs;
  seqTs.SetSeq (pktCount);
  pktCount+=1;
  p->AddHeader (seqTs); //confirm if size of seqTs is 12 or not??
  socket->Send(p); // means sent for Contention

  //   {
  //   socket->Close ();
  //   MsgCount = 0;
  //   }
}


double x_error; double y_error; double z_error; double total_error;
int32_t total_received_packets = 0;
void ReceivePacket (Ptr<Socket> socket)
{
  Ptr<Packet> packet;
  Time rcv;
  Time sqhd;
  Time total_delay;
  while ((packet = socket->Recv ()))
  {
    total_received_packets++;
    SeqTsHeader seqTsx; 
    packet->RemoveHeader(seqTsx);
    rcv = Simulator::Now(); // time of reception
    sqhd = seqTsx.GetTs(); //time stamp of when packet generated
    total_delay = (rcv - sqhd); //total delay calculation
    loc_header new_header;
    packet->RemoveHeader(new_header);
    Vector old_position = new_header.GetLocation();
    uint32_t SenderId = new_header.GetSenderId();
    uint32_t MsgId = new_header.GetMsgId();
    Ptr<Node> Sender_Node = NodeList::GetNode (SenderId);
    double xx; //double yy; double zz; // will store positions
    Ptr<ConstantVelocityMobilityModel> mob2 = socket->GetNode()->GetObject<ConstantVelocityMobilityModel>();
    double xx2 = mob2->GetPosition().x; //receiver's position

    // Ptr<MobilityModel> mob1 = Sender_Node->GetObject<MobilityModel>();
    Ptr<ConstantVelocityMobilityModel> mob1 = Sender_Node->GetObject<ConstantVelocityMobilityModel>();
    double xx1 = mob1->GetVelocity().x; //double yy1 = mob1->GetVelocity().y; double zz1 = mob1->GetVelocity().z;
    // std::cout<<"Old position : "<<old_position.x<<", "<<old_position.y<<", "<<old_position.z<<std::endl;
    // std::cout<<"Actual positions are : "<<xx<<", "<<yy<<", "<<zz<<"\n"<<std::endl;
    double old_x = old_position.x; //double old_y = old_position.y; double old_z = old_position.z;;
    //double x_error = old_x - xx; //double y_error = old_y - yy; double z_error = old_z - zz; 
    // total_error = sqrt(pow(x_error, 2) + pow(y_error, 2) + pow(z_error, 2));
    xx = mob1->GetPosition().x; //yy = mob1->GetPosition().y; zz = mob1->GetPosition().z;
    // total_error = sqrt(pow(x_error, 2)); // as velocity is only along x-axis
    total_error = abs(xx-old_x);
    // freopen(file_name.c_str(),"a",stdout); //https://stackoverflow.com/questions/21589353/cannot-convert-stdbasic-stringchar-to-const-char-for-argument-1-to-i
    // std::cout<<"MsgId = "<<MsgId<<", SenderId = "<<SenderId<<", ReceiverId = "<<socket->GetNode()->GetId()<<", x = "<<old_position.x<<", y = "<<old_position.y<<", z = "<<old_position.z<<"\n"<<std::endl;
    std::cout<<MsgId<<" "<<SenderId<<" "<<socket->GetNode()->GetId()<<" "<<total_error<<" "<<sqhd.GetInteger()<<" "<<rcv.GetInteger()<<" "<<total_delay.GetInteger()<<" "<<xx<<" "<<old_x<<" "<<xx1<<" "<<xx2<<"\n"<<std::endl;
  }
}

// This trace will log packet transmissions and receptions from the application
// layer.  The parameter 'localAddrs' is passed to this trace in case the
// address passed by the trace is not set (i.e., is '0.0.0.0' or '::').  The
// trace writes to a file stream provided by the first argument; by default,
// this trace file is 'UePacketTrace.tr'
void
UePacketTrace (Ptr<OutputStreamWrapper> stream, const Address &localAddrs, std::string context, Ptr<const Packet> p, const Address &srcAddrs, const Address &dstAddrs)
{
  std::cout << "In UEPacket Trace\n";
  std::ostringstream oss;
  *stream->GetStream () << Simulator::Now ().GetNanoSeconds () / (double) 1e9 << "\t" << context << "\t" << p->GetSize () << "\t";
  if (InetSocketAddress::IsMatchingType (srcAddrs))
    {
      oss << InetSocketAddress::ConvertFrom (srcAddrs).GetIpv4 ();
      if (!oss.str ().compare ("0.0.0.0")) //srcAddrs not set
        {
          *stream->GetStream () << Ipv4Address::ConvertFrom (localAddrs) << ":" << InetSocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t" << InetSocketAddress::ConvertFrom (dstAddrs).GetIpv4 () << ":" << InetSocketAddress::ConvertFrom (dstAddrs).GetPort () << std::endl;
        }
      else
        {
          oss.str ("");
          oss << InetSocketAddress::ConvertFrom (dstAddrs).GetIpv4 ();
          if (!oss.str ().compare ("0.0.0.0")) //dstAddrs not set
            {
              *stream->GetStream () << InetSocketAddress::ConvertFrom (srcAddrs).GetIpv4 () << ":" << InetSocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t" <<  Ipv4Address::ConvertFrom (localAddrs) << ":" << InetSocketAddress::ConvertFrom (dstAddrs).GetPort () << std::endl;
            }
          else
            {
              *stream->GetStream () << InetSocketAddress::ConvertFrom (srcAddrs).GetIpv4 () << ":" << InetSocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t" << InetSocketAddress::ConvertFrom (dstAddrs).GetIpv4 () << ":" << InetSocketAddress::ConvertFrom (dstAddrs).GetPort () << std::endl;
            }
        }
    }
  else if (Inet6SocketAddress::IsMatchingType (srcAddrs))
    {
      oss << Inet6SocketAddress::ConvertFrom (srcAddrs).GetIpv6 ();
      if (!oss.str ().compare ("::")) //srcAddrs not set
        {
          *stream->GetStream () << Ipv6Address::ConvertFrom (localAddrs) << ":" << Inet6SocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t" << Inet6SocketAddress::ConvertFrom (dstAddrs).GetIpv6 () << ":" << Inet6SocketAddress::ConvertFrom (dstAddrs).GetPort () << std::endl;
        }
      else
        {
          oss.str ("");
          oss << Inet6SocketAddress::ConvertFrom (dstAddrs).GetIpv6 ();
          if (!oss.str ().compare ("::")) //dstAddrs not set
            {
              *stream->GetStream () << Inet6SocketAddress::ConvertFrom (srcAddrs).GetIpv6 () << ":" << Inet6SocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t" << Ipv6Address::ConvertFrom (localAddrs) << ":" << Inet6SocketAddress::ConvertFrom (dstAddrs).GetPort () << std::endl;
            }
          else
            {
              *stream->GetStream () << Inet6SocketAddress::ConvertFrom (srcAddrs).GetIpv6 () << ":" << Inet6SocketAddress::ConvertFrom (srcAddrs).GetPort () << "\t" << Inet6SocketAddress::ConvertFrom (dstAddrs).GetIpv6 () << ":" << Inet6SocketAddress::ConvertFrom (dstAddrs).GetPort () << std::endl;
            }
        }
    }
  else
    {
      *stream->GetStream () << "Unknown address type!" << std::endl;
    }
}

/*
 * The topology is the following:
 *
 *          UE1..........(20 m)..........UE2
 *   (0.0, 0.0, 1.5)            (20.0, 0.0, 1.5)
 *
 * Please refer to the Sidelink section of the LTE user documentation for more details.
 *
 */

NS_LOG_COMPONENT_DEFINE ("myLteSlOutOfCovrg");

int main (int argc, char *argv[])
{
  Time simTime = Seconds (20);
  bool enableNsLogs = false;
  bool useIPv6 = false;

  CommandLine cmd;
  cmd.AddValue ("simTime", "Total duration of the simulation", simTime);
  cmd.AddValue ("enableNsLogs", "Enable ns-3 logging (debug builds)", enableNsLogs);
  cmd.AddValue ("useIPv6", "Use IPv6 instead of IPv4", useIPv6);
  cmd.Parse (argc, argv);

  /**** SECTION 1 **** Configuration of LTE and Sidelink default Paramters ***/

  // Pareneters for PSSCH resource selection
  Config::SetDefault ("ns3::LteUeMac::SlGrantMcs", UintegerValue (16)); // Fixed MCS
  Config::SetDefault ("ns3::LteUeMac::SlGrantSize", UintegerValue (5)); // The number of RBs allocated per UE for Sidelink
  Config::SetDefault ("ns3::LteUeMac::Ktrp", UintegerValue (1)); // For selecting subframe indicator bitmap
  Config::SetDefault ("ns3::LteUeMac::UseSetTrp", BooleanValue (true)); // Use default TRP index of 0

  // Frequency
  uint32_t ulEarfcn = 18100;
  uint16_t ulBandwidth = 50;

  // Error models
  Config::SetDefault ("ns3::LteSpectrumPhy::SlCtrlErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::SlDataErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::LteSpectrumPhy::DropRbOnCollisionEnabled", BooleanValue (false));

  // Transmit power for the UEs
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (23.0));

  // TODO screen
  // parse again so we can override input file default values via command line
  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults ();
  cmd.Parse (argc, argv);
  if (enableNsLogs)
    {
      LogLevel logLevel = (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_TIME | LOG_PREFIX_NODE | LOG_LEVEL_ALL);
      LogComponentEnable ("LteUeRrc", logLevel);
      LogComponentEnable ("LteUeMac", logLevel);
      LogComponentEnable ("LteSpectrumPhy", logLevel);
      LogComponentEnable ("LteUePhy", logLevel);
    }

  // TODO relocate
  //Sidelink bearers activation time
  Time slBearersActivationTime = Seconds (2.0);


  /**** SECTION 2 **** Topology configuration ***/

  // Instatiating LTE, EPC, and Sidelink helpers
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  Ptr<LteSidelinkHelper> proseHelper = CreateObject<LteSidelinkHelper> ();
  proseHelper->SetLteHelper (lteHelper);

  // Enabling the Sidelink
  lteHelper->SetAttribute ("UseSidelink", BooleanValue (true));

  // Configuring the pathloss model and bypass the use of eNB nodes
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::Cost231PropagationLossModel"));
  lteHelper->Initialize (); // channel model initialization
  double ulFreq = LteSpectrumValueHelper::GetCarrierFrequency (ulEarfcn); // 18100
  NS_LOG_LOGIC ("UL freq: " << ulFreq);
  Ptr<Object> uplinkPathlossModel = lteHelper->GetUplinkPathlossModel ();
  Ptr<PropagationLossModel> lossModel = uplinkPathlossModel->GetObject<PropagationLossModel> ();
  NS_ABORT_MSG_IF (lossModel == nullptr, "No PathLossModel");
  bool ulFreqOk = uplinkPathlossModel->SetAttributeFailSafe ("Frequency", DoubleValue (ulFreq));
  if (!ulFreqOk)
    {
      NS_LOG_WARN ("UL propagation model does not have a Frequency attribute");
    }

  // Creating the UE nodes and setting their mobility
  NS_LOG_INFO ("Deploying UE's...");
  NodeContainer ueNodes;
  ueNodes.Create (3);
  NS_LOG_INFO ("UE 1 node id = [" << ueNodes.Get (0)->GetId () << "]");
  NS_LOG_INFO ("UE 2 node id = [" << ueNodes.Get (1)->GetId () << "]");
  NS_LOG_INFO ("UE 3 node id = [" << ueNodes.Get (2)->GetId () << "]");

  Ptr<ListPositionAllocator> positionAllocUe1 = CreateObject<ListPositionAllocator> (); // Position of the nodes
  positionAllocUe1->Add (Vector (0.0, 0.0, 1.5)); // TODO units?

  Ptr<ListPositionAllocator> positionAllocUe2 = CreateObject<ListPositionAllocator> ();
  positionAllocUe2->Add (Vector (20.0, 0.0, 1.5));

  Ptr<ListPositionAllocator> positionAllocUe3 = CreateObject<ListPositionAllocator> ();
  positionAllocUe3->Add (Vector (10.0, 0.0, 1.5));

  MobilityHelper mobilityUe1; // Install mobility
  mobilityUe1.SetMobilityModel ("ns3::ConstantVelocityMobilityModel"); // TODO make mobile
  mobilityUe1.SetPositionAllocator (positionAllocUe1);
  mobilityUe1.Install (ueNodes.Get (0));

  MobilityHelper mobilityUe2;
  mobilityUe2.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  mobilityUe2.SetPositionAllocator (positionAllocUe2);
  mobilityUe2.Install (ueNodes.Get (1));

  MobilityHelper mobilityUe3;
  mobilityUe2.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  mobilityUe2.SetPositionAllocator (positionAllocUe3);
  mobilityUe2.Install (ueNodes.Get (2));

  // Installing LTE UE devices to the nodes
  NetDeviceContainer ueDevs = lteHelper->InstallUeDevice (ueNodes);
 
  // TODO relocate
  // Fix the random number stream
  uint16_t randomStream = 1;
  randomStream += lteHelper->AssignStreams (ueDevs, randomStream);


  /**** SECTION 2 **** Sidelink Pool Configuration ***/
  // TODO investiate SLpool, regarding send data

  Ptr<LteSlUeRrc> ueSidelinkConfiguration = CreateObject<LteSlUeRrc> (); // Sidelink pre-configuration for the UEs
  ueSidelinkConfiguration->SetSlEnabled (true);

  LteRrcSap::SlPreconfiguration preconfiguration;
  preconfiguration.preconfigGeneral.carrierFreq = ulEarfcn;
  preconfiguration.preconfigGeneral.slBandwidth = ulBandwidth;
  preconfiguration.preconfigComm.nbPools = 1;
  LteSlPreconfigPoolFactory pfactory;
  
  pfactory.SetControlPeriod ("sf40");   //Control
  pfactory.SetControlBitmap (0x00000000FF); // 8 subframes for PSCCH
  pfactory.SetControlOffset (0);
  pfactory.SetControlPrbNum (22);
  pfactory.SetControlPrbStart (0);
  pfactory.SetControlPrbEnd (49);

  pfactory.SetDataBitmap (0xFFFFFFFFFF);   //Data
  pfactory.SetDataOffset (8); // After 8 subframes of PSCCH
  pfactory.SetDataPrbNum (25);
  pfactory.SetDataPrbStart (0);
  pfactory.SetDataPrbEnd (49);

  preconfiguration.preconfigComm.pools[0] = pfactory.CreatePool ();
  preconfiguration.preconfigComm.nbPools = 1;

  ueSidelinkConfiguration->SetSlPreconfiguration (preconfiguration);
  lteHelper->InstallSidelinkConfiguration (ueDevs, ueSidelinkConfiguration);


  /**** SECTION 4 **** IP Configuration ***/

  // Installing the IP stack on the UEs and assigning IP addresses (IPv4 code statements shown below)
  InternetStackHelper internet;
  internet.Install (ueNodes);
  uint32_t groupL2Address = 255;
  Ipv4Address groupAddress4 ("225.255.255.255"); // use multicast address as destination
  Ipv6Address groupAddress6 ("ff0e::1"); // use multicast address as destination
  Address remoteAddress;
  Address localAddress;
  Address localAddress2;
  Ptr<LteSlTft> tft;
  if (!useIPv6)
    { // TODO screen
      Ipv4InterfaceContainer ueIpIface;
      ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueDevs));

      // set the default gateway for the UE
      Ipv4StaticRoutingHelper ipv4RoutingHelper;
      for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
        {
          Ptr<Node> ueNode = ueNodes.Get (u);
          // Set the default gateway for the UE
          Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
          ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
        }
      remoteAddress = InetSocketAddress (groupAddress4, 8000);
      localAddress = InetSocketAddress (Ipv4Address::GetAny (), 8000);
      localAddress2 = InetSocketAddress (Ipv4Address::GetAny (), 8000);
      tft = Create<LteSlTft> (LteSlTft::BIDIRECTIONAL, groupAddress4, groupL2Address); // TODO investigate
    }
  else
    {
      Ipv6InterfaceContainer ueIpIface;
      ueIpIface = epcHelper->AssignUeIpv6Address (NetDeviceContainer (ueDevs));

      // set the default gateway for the UE
      Ipv6StaticRoutingHelper ipv6RoutingHelper;
      for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
        {
          Ptr<Node> ueNode = ueNodes.Get (u);
          // Set the default gateway for the UE
          Ptr<Ipv6StaticRouting> ueStaticRouting = ipv6RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv6> ());
          ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress6 (), 1);
        }
      remoteAddress = Inet6SocketAddress (groupAddress6, 8000);
      localAddress = Inet6SocketAddress (Ipv6Address::GetAny (), 8000);
      tft = Create<LteSlTft> (LteSlTft::BIDIRECTIONAL, groupAddress6, groupL2Address);
    }

  /**** SECTION 5 **** Application Configuration ***/
  // TODO investigate application options

  // Installing applications and activating Sidelink radio bearers
  // OnOffHelper sidelinkClient = MyApplication();

  // below for custom appp
  // MyApplication sidelinkClient = MyApplication();
  // sidelinkClient.SetAttribute("DataRate", DataRateValue(DataRate("16kb/s")));
  // sidelinkClient.SetAttribute("Remote", AddressValue(remoteAddress));

  //("ns3::UdpSocketFactory", remoteAddress); // Set Application in the UEs
  // sidelinkClient.SetConstantRate (DataRate ("16kb/s"), 200); // datarate & packet-size

  // ApplicationContainer clientApps = sidelinkClient.Install (ueNodes.Get (0));
  //onoff application will send the first packet at :
  //(2.9 (App Start Time) + (1600 (Pkt size in bits) / 16000 (Data rate)) = 3.0 sec
  // clientApps.Start (slBearersActivationTime + Seconds (0.9));
  // clientApps.Stop (simTime - slBearersActivationTime + Seconds (1.0));

  // ApplicationContainer serverApps;
  // PacketSinkHelper sidelinkSink ("ns3::UdpSocketFactory", localAddress);
  // serverApps = sidelinkSink.Install (ueNodes); // make all nodes listen
  // serverApps.Start ( ns3::Now());
  // serverApps.Add(sidelinkSink.Install (ueNodes.Get (2)));

  proseHelper->ActivateSidelinkBearer (slBearersActivationTime, ueDevs, tft); // Set Sidelink bearers

  int nSinks = 3; // number of nodes
  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");

  // set up sinks for each node and create socket to send
  for (int i = 0; i < nSinks; i++)
    {
        Ptr<Socket> recvSink = Socket::CreateSocket (ueNodes.Get (i), tid);
        InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 8000);
        recvSink->Bind (local);
        recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));
        sink_map[i] = recvSink;

        Ptr<Socket> source = Socket::CreateSocket (ueNodes.Get (i), tid);
        source->SetAllowBroadcast (true);
        source->Connect (remoteAddress);
        source_map[i] = source;
    }

  ///*** End of application configuration ***///

  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream ("UePacketTrace.tr");

  //Trace file table header
  *stream->GetStream () << "time(sec)\ttx/rx\tNodeID\tIMSI\tPktSize(bytes)\tIP[src]\tIP[dst]" << std::endl;

  std::ostringstream oss;

  if (!useIPv6)
    {
      // Set Tx traces
      for (uint16_t ac = 0; ac < source_map.size (); ac++)
        {
          Ipv4Address localAddrs =  source_map[ac]->GetNode ()->GetObject<Ipv4L3Protocol> ()->GetAddress (1,0).GetLocal ();
          std::cout << "Tx address: " << localAddrs << std::endl;
          
          std::cout << "Setting packet trace:\ntx\t" << ueNodes.Get (ac)->GetId () << "\t" << ueNodes.Get (ac)->GetDevice (0)->GetObject<LteUeNetDevice> ()->GetImsi ()<<"\n";

          oss << "tx\t" << ueNodes.Get (ac)->GetId () << "\t" << ueNodes.Get (ac)->GetDevice (0)->GetObject<LteUeNetDevice> ()->GetImsi ();
          
          (source_map[ac])->TraceConnect ("TxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, stream, localAddrs));
          oss.str ("");
        }

      // Set Rx traces
      for (uint16_t ac = 0; ac < source_map.size(); ac++)
        {
          Ipv4Address localAddrs =  source_map[ac]->GetNode ()->GetObject<Ipv4L3Protocol> ()->GetAddress (1,0).GetLocal ();
          std::cout << "Rx address: " << localAddrs << std::endl;
          oss << "rx\t" << ueNodes.Get (ac)->GetId () << "\t" << ueNodes.Get (ac)->GetDevice (0)->GetObject<LteUeNetDevice> ()->GetImsi ();
          source_map[ac]->TraceConnect ("RxWithAddresses", oss.str (), MakeBoundCallback (&UePacketTrace, stream, localAddrs));
          oss.str ("");
        }
    }

  


  uint32_t packetSize = 320; // bytes

  std::cout << "Scheduling sendPacket with node: " << source_map[0]->GetNode()->GetId() << " sending 1 packet at 5sec\n"; 
  Simulator::ScheduleWithContext (source_map[0]->GetNode()->GetId(),
                                    ns3::Seconds(5.0), &sendPacket,
                                    source_map[0], packetSize); //without rv, no packets delivered

  std::cout << "Scheduling sendPacket with node: " << source_map[1]->GetNode()->GetId() << " sending 1 packet at 10sec\n"; 
  Simulator::ScheduleWithContext (source_map[1]->GetNode()->GetId(),
                                    ns3::Seconds(10.0), &sendPacket,
                                    source_map[1], packetSize); 

  std::cout << "Scheduling sendPacket with node: " << source_map[2]->GetNode()->GetId() << " sending 1 packet at 15sec\n"; 
  Simulator::ScheduleWithContext (source_map[2]->GetNode()->GetId(),
                                    ns3::Seconds(15.0), &sendPacket,
                                    source_map[2], packetSize); 

  NS_LOG_INFO ("Enabling Sidelink traces...");
  lteHelper->EnableSidelinkTraces ();

  NS_LOG_INFO ("Starting simulation...");

  Simulator::Stop (simTime);

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;

}