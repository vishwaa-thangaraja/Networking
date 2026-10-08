#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/error-model.h"

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <cstdlib>

using namespace ns3;
using namespace std;

/* -------------------------
   GLOBALS FOR TRACING
   ------------------------- */

static ofstream g_cwndFile;
static ofstream g_metricsFile;

static Ptr<FlowMonitor> g_flowmon;
static Ptr<Ipv4FlowClassifier> g_classifier;

static uint16_t g_port = 5000;
static double g_interval = 0.5;   // sampling interval in seconds

static uint64_t g_prevRxBytes = 0;
static uint64_t g_prevRxPackets = 0;
static uint64_t g_prevTxPackets = 0;
static double g_prevDelaySum = 0.0;

/* Called every time the congestion window changes */
static void
CwndTracer(uint32_t oldCwnd, uint32_t newCwnd)
{
    g_cwndFile << Simulator::Now().GetSeconds() << "," << newCwnd << endl;
}

/* Connect the cwnd trace (the socket only exists after the app starts) */
static void
ConnectCwndTrace(uint32_t nodeId)
{
    Config::ConnectWithoutContext("/NodeList/" + to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeCallback(&CwndTracer));
}

/* Throughput, packet loss and delay for each time interval */
static void
SampleMetrics()
{
    FlowMonitor::FlowStatsContainer stats = g_flowmon->GetFlowStats();

    for (auto &it : stats)
    {
        Ipv4FlowClassifier::FiveTuple t = g_classifier->FindFlow(it.first);

        if (t.protocol != 6)
            continue;

        if (t.destinationPort != g_port)
            continue;

        const FlowMonitor::FlowStats &st = it.second;

        uint64_t dRxBytes   = st.rxBytes   - g_prevRxBytes;
        uint64_t dRxPackets = st.rxPackets - g_prevRxPackets;
        uint64_t dTxPackets = st.txPackets - g_prevTxPackets;
        double   dDelay     = st.delaySum.GetSeconds() - g_prevDelaySum;

        double thr = dRxBytes * 8.0 / g_interval / 1000000.0;

        double loss = 0.0;
        if (dTxPackets > dRxPackets && dTxPackets > 0)
        {
            loss = ((double)(dTxPackets - dRxPackets) / dTxPackets) * 100.0;
        }

        double delay = 0.0;
        if (dRxPackets > 0)
        {
            delay = dDelay / dRxPackets;
        }

        g_metricsFile << Simulator::Now().GetSeconds() << ","
                      << thr << ","
                      << loss << ","
                      << delay << endl;

        g_prevRxBytes   = st.rxBytes;
        g_prevRxPackets = st.rxPackets;
        g_prevTxPackets = st.txPackets;
        g_prevDelaySum  = st.delaySum.GetSeconds();
    }

    Simulator::Schedule(Seconds(g_interval), &SampleMetrics);
}

/* Set TCP algorithm */
void
SetTcpType(Ptr<Node> node, string tcpType)
{
    TypeId tid = TypeId::LookupByName("ns3::" + tcpType);
    string path = "/NodeList/" + to_string(node->GetId()) + "/$ns3::TcpL4Protocol/SocketType";
    Config::Set(path, TypeIdValue(tid));
}

/* Main */
int
main(int argc, char *argv[])
{
    string tcpType = "TcpNewReno";
    string bottleneckRate = "1Mbps";

    double errorRate = 0.05;
    double simulationTime = 20.0;

    /* Command line */
    CommandLine cmd;
    cmd.AddValue("tcpType", "TCP algorithm", tcpType);
    cmd.AddValue("bottleneckRate", "Bottleneck bandwidth", bottleneckRate);
    cmd.AddValue("errorRate", "Packet error rate", errorRate);
    cmd.AddValue("simulationTime", "Simulation time", simulationTime);
    cmd.Parse(argc, argv);

    /* -------------------------
       1. NODE CREATION
       ------------------------- */
    NodeContainer senders;
    NodeContainer routers;
    NodeContainer receivers;

    senders.Create(4);
    routers.Create(2);
    receivers.Create(4);

    cout << "10 nodes created" << endl;

    /* -------------------------
       2. LINK CREATION
       ------------------------- */
    PointToPointHelper access;
    access.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    access.SetChannelAttribute("Delay", StringValue("2ms"));

    PointToPointHelper bottleneck;
    bottleneck.SetDeviceAttribute("DataRate", StringValue(bottleneckRate));
    bottleneck.SetChannelAttribute("Delay", StringValue("10ms"));
    bottleneck.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("50p"));

    /* Sender 0 to Router 0 */
    NetDeviceContainer s0r0 = access.Install(senders.Get(0), routers.Get(0));

    /* Sender 1 to Router 0 */
    NetDeviceContainer s1r0 = access.Install(senders.Get(1), routers.Get(0));

    /* Sender 2 to Router 0 */
    NetDeviceContainer s2r0 = access.Install(senders.Get(2), routers.Get(0));

    /* Sender 3 to Router 0 */
    NetDeviceContainer s3r0 = access.Install(senders.Get(3), routers.Get(0));

    /* Router 1 to Receiver 0 */
    NetDeviceContainer r1d0 = access.Install(routers.Get(1), receivers.Get(0));

    /* Router 1 to Receiver 1 */
    NetDeviceContainer r1d1 = access.Install(routers.Get(1), receivers.Get(1));

    /* Router 1 to Receiver 2 */
    NetDeviceContainer r1d2 = access.Install(routers.Get(1), receivers.Get(2));

    /* Router 1 to Receiver 3 */
    NetDeviceContainer r1d3 = access.Install(routers.Get(1), receivers.Get(3));

    /* Bottleneck */
    NetDeviceContainer bottleneckDevices = bottleneck.Install(routers.Get(0), routers.Get(1));

    cout << "Access links created" << endl;
    cout << "Bottleneck link created at " << bottleneckRate << endl;

    /* -------------------------
       3. INTERNET STACK
       ------------------------- */
    InternetStackHelper stack;
    stack.Install(senders);
    stack.Install(routers);
    stack.Install(receivers);

    /* Select TCP algorithm */
    SetTcpType(senders.Get(0), tcpType);

    /* -------------------------
       4. IP ADDRESSES
       ------------------------- */
    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer s0Interface = address.Assign(s0r0);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer s1Interface = address.Assign(s1r0);

    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer s2Interface = address.Assign(s2r0);

    address.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer s3Interface = address.Assign(s3r0);

    address.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer bottleneckInterface = address.Assign(bottleneckDevices);

    address.SetBase("10.1.6.0", "255.255.255.0");
    Ipv4InterfaceContainer r0Interface = address.Assign(r1d0);

    address.SetBase("10.1.7.0", "255.255.255.0");
    Ipv4InterfaceContainer r1Interface = address.Assign(r1d1);

    address.SetBase("10.1.8.0", "255.255.255.0");
    Ipv4InterfaceContainer r2Interface = address.Assign(r1d2);

    address.SetBase("10.1.9.0", "255.255.255.0");
    Ipv4InterfaceContainer r3Interface = address.Assign(r1d3);

    /* -------------------------
       5. ERROR MODEL
       ------------------------- */
    Ptr<RateErrorModel> errorModel = CreateObject<RateErrorModel>();
    errorModel->SetAttribute("ErrorRate", DoubleValue(errorRate));
    errorModel->SetAttribute("ErrorUnit", EnumValue(RateErrorModel::ERROR_UNIT_PACKET));

    bottleneckDevices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(errorModel));

    cout << "Packet error rate = " << errorRate << endl;

    /* -------------------------
       6. ROUTING
       ------------------------- */
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    /* -------------------------
       7. TCP SINK
       ------------------------- */
    uint16_t port = g_port;

    PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApp = sink.Install(receivers.Get(0));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(simulationTime));

    /* -------------------------
       8. TCP SOURCE
       ------------------------- */
    BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(r0Interface.GetAddress(1), port));
    source.SetAttribute("MaxBytes", UintegerValue(0));
    source.SetAttribute("SendSize", UintegerValue(1024));

    ApplicationContainer sourceApp = source.Install(senders.Get(0));
    sourceApp.Start(Seconds(1.0));
    sourceApp.Stop(Seconds(simulationTime - 1.0));

    /* -------------------------
       9. FLOW MONITOR + TRACING
       ------------------------- */
    FlowMonitorHelper flowmonHelper;
    Ptr<FlowMonitor> flowmon = flowmonHelper.InstallAll();
    g_flowmon = flowmon;
    g_classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());

    /* Output files (one pair per TCP algorithm) */
    g_cwndFile.open("cwnd_" + tcpType + ".csv");
    g_cwndFile << "Time,Cwnd" << endl;

    g_metricsFile.open("metrics_" + tcpType + ".csv");
    g_metricsFile << "Time,Throughput,PacketLoss,Delay" << endl;

    /* Source starts at 1.0 s, so connect the cwnd trace just after */
    Simulator::Schedule(Seconds(1.001), &ConnectCwndTrace, senders.Get(0)->GetId());

    /* Start periodic sampling of throughput, loss and delay */
    Simulator::Schedule(Seconds(1.0 + g_interval), &SampleMetrics);

    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    /* -------------------------
       10. OUTPUT
       ------------------------- */
    flowmon->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = flowmon->GetFlowStats();

    double throughput = 0.0;
    double packetLoss = 0.0;
    double averageDelay = 0.0;

    for (map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i)
    {
        Ipv4FlowClassifier::FiveTuple tuple = classifier->FindFlow(i->first);

        if (tuple.protocol != 6)
            continue;

        if (tuple.destinationPort != port)
            continue;

        const FlowMonitor::FlowStats &st = i->second;

        double duration = (st.timeLastRxPacket - st.timeFirstRxPacket).GetSeconds();

        if (duration > 0.0)
        {
            throughput = (st.rxBytes * 8.0) / duration / 1000000.0;
        }

        if (st.txPackets > 0)
        {
            packetLoss = ((double)st.lostPackets / st.txPackets) * 100.0;
        }

        if (st.rxPackets > 0)
        {
            averageDelay = st.delaySum.GetSeconds() / st.rxPackets;
        }

        cout << endl;
        cout << "========== FLOW RESULT ==========" << endl;
        cout << "TCP Algorithm     : " << tcpType << endl;
        cout << "Throughput        : " << throughput << " Mbps" << endl;
        cout << "Tx Packets        : " << st.txPackets << endl;
        cout << "Rx Packets        : " << st.rxPackets << endl;
        cout << "Lost Packets      : " << st.lostPackets << endl;
        cout << "Packet Loss       : " << packetLoss << " %" << endl;
        cout << "Average Delay     : " << averageDelay << " seconds" << endl;
    }

    /* -------------------------
       FINAL SUMMARY
       ------------------------- */
    cout << endl;
    cout << "========== RESULT ==========" << endl;
    cout << "Number of Nodes   : 10" << endl;
    cout << "TCP Algorithm     : " << tcpType << endl;
    cout << "Bottleneck Rate   : " << bottleneckRate << endl;
    cout << "Packet Error Rate : " << errorRate << endl;
    cout << "Throughput        : " << throughput << " Mbps" << endl;
    cout << "Packet Loss       : " << packetLoss << " %" << endl;
    cout << "Average Delay     : " << averageDelay << " seconds" << endl;

    g_cwndFile.close();
    g_metricsFile.close();

    Simulator::Destroy();

    // ==========================================
    // MENU INJECTED AT THE END
    // ==========================================
    while(true) {
        cout << "\n========== NS-3 MENU ==========\n";
        cout << "1. Display/Generate Graph\n";
        cout << "2. Exit\n";
        cout << "Enter your choice: ";
        
        int choice;
        if (!(cin >> choice)) {
            break;
        }
        
        if (choice == 1) {
            cout << "Generating Graph...\n";
            int ret = system("python3 plot.py");
            if (ret != 0) {
                // Try fallback to just python if python3 fails
                system("python plot.py");
            }
        } else if (choice == 2) {
            cout << "Exiting...\n";
            break;
        } else {
            cout << "Invalid choice! Please enter 1 or 2.\n";
        }
    }

    return 0;
}
