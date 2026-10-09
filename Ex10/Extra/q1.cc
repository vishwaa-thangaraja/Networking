#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpCongestionQ1");

static void CwndChange(uint32_t oldCwnd, uint32_t newCwnd) {
    std::cout << Simulator::Now().GetSeconds() << "\t" << newCwnd << std::endl;
}

static void TraceCwnd() {
    Config::ConnectWithoutContext("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback(&CwndChange));
}

int main(int argc, char *argv[]) {
    // 1. Create nodes: 1 Sender, 1 Router, 1 Receiver
    NodeContainer nodes;
    nodes.Create(3);

    // 2. Create point-to-point links
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    p2p.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("50p"));

    NetDeviceContainer dev01 = p2p.Install(nodes.Get(0), nodes.Get(1)); // Sender to Router
    NetDeviceContainer dev12 = p2p.Install(nodes.Get(1), nodes.Get(2)); // Router to Receiver

    // 3. Install Internet stack
    InternetStackHelper stack;
    stack.Install(nodes);

    // 4. Assign IP addresses
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer if01 = address.Assign(dev01);
    
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer if12 = address.Assign(dev12);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // 5. Setup TCP Receiver (Node 2)
    uint16_t port = 8080;
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sinkHelper.Install(nodes.Get(2));
    sinkApp.Start(Seconds(1.0));
    sinkApp.Stop(Seconds(10.0));

    // 6. Setup TCP Sender (Node 0) generating traffic
    BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(if12.GetAddress(1), port));
    source.SetAttribute("MaxBytes", UintegerValue(0)); // Send indefinitely
    ApplicationContainer sourceApp = source.Install(nodes.Get(0));
    sourceApp.Start(Seconds(1.0));
    sourceApp.Stop(Seconds(10.0));

    // 7. Schedule TraceCwnd right after socket is created by the application
    Simulator::Schedule(Seconds(1.00001), &TraceCwnd);

    // 8. Run simulation
    Simulator::Stop(Seconds(10.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
