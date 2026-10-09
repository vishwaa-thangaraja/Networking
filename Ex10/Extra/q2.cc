#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/error-model.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpCongestionCompareQ2");

static void CwndChangeNewReno(uint32_t oldCwnd, uint32_t newCwnd) {
    std::cout << "NewReno CWND Time: " << Simulator::Now().GetSeconds() << " val: " << newCwnd << std::endl;
}

static void CwndChangeCubic(uint32_t oldCwnd, uint32_t newCwnd) {
    std::cout << "Cubic CWND Time: " << Simulator::Now().GetSeconds() << " val: " << newCwnd << std::endl;
}

static void TraceCwnd() {
    Config::ConnectWithoutContext("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback(&CwndChangeNewReno));
    Config::ConnectWithoutContext("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback(&CwndChangeCubic));
}

int main(int argc, char *argv[]) {
    // 1. Create nodes: 2 Senders, 2 Routers, 1 Receiver
    NodeContainer senders, routers, receiver;
    senders.Create(2); // Node 0: NewReno, Node 1: Cubic
    routers.Create(2); // Router 0 and Router 1
    receiver.Create(1); // Node 2: Receiver

    // 2. Install Internet stack and set specific TCP variants
    InternetStackHelper stack;
    
    // Set NewReno for Sender 0
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));
    stack.Install(senders.Get(0));
    
    // Set Cubic for Sender 1
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
    stack.Install(senders.Get(1));

    // Install default (NewReno) on routers and receiver
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpNewReno"));
    stack.Install(routers);
    stack.Install(receiver);

    // 3. Create point-to-point links
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    p2p.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("100p"));

    PointToPointHelper bottleneck;
    bottleneck.SetDeviceAttribute("DataRate", StringValue("2Mbps")); // Shared bottleneck
    bottleneck.SetChannelAttribute("Delay", StringValue("10ms"));
    bottleneck.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("50p"));

    // Connect Senders to Router 0
    NetDeviceContainer dev0R0 = p2p.Install(senders.Get(0), routers.Get(0));
    NetDeviceContainer dev1R0 = p2p.Install(senders.Get(1), routers.Get(0));
    
    // Connect Router 0 to Router 1 (Bottleneck)
    NetDeviceContainer devR0R1 = bottleneck.Install(routers.Get(0), routers.Get(1));
    
    // Introduce packet error rate on bottleneck to induce congestion
    Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
    em->SetAttribute("ErrorRate", DoubleValue(0.0001));
    devR0R1.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

    // Connect Router 1 to Receiver
    NetDeviceContainer devR1Recv = p2p.Install(routers.Get(1), receiver.Get(0));

    // 4. Assign IP addresses
    Ipv4AddressHelper address;
    
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer if0R0 = address.Assign(dev0R0);
    
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer if1R0 = address.Assign(dev1R0);

    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer ifR0R1 = address.Assign(devR0R1);

    address.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer ifR1Recv = address.Assign(devR1Recv);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // 5. Setup Receivers (Sinks)
    uint16_t portNewReno = 8080;
    uint16_t portCubic = 8081;
    
    PacketSinkHelper sinkNewReno("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), portNewReno));
    PacketSinkHelper sinkCubic("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), portCubic));
    
    ApplicationContainer sinkApps;
    sinkApps.Add(sinkNewReno.Install(receiver.Get(0)));
    sinkApps.Add(sinkCubic.Install(receiver.Get(0)));
    sinkApps.Start(Seconds(1.0));
    sinkApps.Stop(Seconds(15.0));

    // 6. Setup Senders
    BulkSendHelper sourceNewReno("ns3::TcpSocketFactory", InetSocketAddress(ifR1Recv.GetAddress(1), portNewReno));
    sourceNewReno.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer sourceAppNewReno = sourceNewReno.Install(senders.Get(0));
    sourceAppNewReno.Start(Seconds(1.0));
    sourceAppNewReno.Stop(Seconds(15.0));

    BulkSendHelper sourceCubic("ns3::TcpSocketFactory", InetSocketAddress(ifR1Recv.GetAddress(1), portCubic));
    sourceCubic.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer sourceAppCubic = sourceCubic.Install(senders.Get(1));
    sourceAppCubic.Start(Seconds(1.0));
    sourceAppCubic.Stop(Seconds(15.0));

    // 7. Trace Cwnd
    Simulator::Schedule(Seconds(1.00001), &TraceCwnd);

    // 8. Run simulation
    Simulator::Stop(Seconds(15.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
