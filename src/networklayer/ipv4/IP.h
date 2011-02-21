//
// Copyright (C) 2004 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_IP_H
#define __INET_IP_H

#include "QueueBase.h"
#include "InterfaceTableAccess.h"
#include "RoutingTableAccess.h"
#include "IRoutingTable.h"
#include "ICMPAccess.h"
#include "IPControlInfo.h"
#include "IPDatagram.h"
#include "IPFragBuf.h"
#include "ProtocolMap.h"

class ARPPacket;
class ICMPMessage;

// ICMP type 2, code 4: fragmentation needed, but don't-fragment bit set
const int ICMP_FRAGMENTATION_ERROR_CODE = 4;

/**
 * Implements the IP protocol.
 */
class INET_API IP: public QueueBase {
protected:
	IRoutingTable *rt;
	IRoutingTable *rt1;
	IInterfaceTable *ift;
	ICMPAccess icmpAccess;
	cGate *queueOutGate; // the most frequently used output gate

	// config
	int defaultTimeToLive;
	int defaultMCTimeToLive;
	simtime_t fragmentTimeoutTime;

	// working vars
	long curFragmentId; // counter, used to assign unique fragmentIds to datagrams
	IPFragBuf fragbuf; // fragmentation reassembly buffer
	simtime_t lastCheckTime; // when fragbuf was last checked for state fragments
	ProtocolMapping mapping; // where to send packets after decapsulation
	int future_packets_count;

	// statistics
	int numMulticast;
	int numLocalDeliver;
	int numDropped;
	int numUnroutable;
	int numForwarded_hash_routing_packet;
	int numForwarded_dynamic_routing_packet;
	int numForwarded_direct_routing_packet;
	int numForwarded_probe_packet;
	int numSent_probe_packet;
	int numReceived_probe_packet;
	int numSent_load_packet;
	int numReceived_load_packet;
	int numForwarded_load_packet;
	int route_index;
    int numPackets;
	/*
	 * This two dimensional array contains the number of packets sent from a particular
	 * to a particular destination. Each row represent a host connected to this edge.
	 * And each column represent the destination edge switch.
	 */
	int **numForwarded_per_host;

	int counter_for_interface_table;
	/*
	 * This matrix holds the data like delay path to be taken by the edge and aggregate switch.
	 * The first dimension represent each edge switch. The second dimension represent the number
	 * of parallel paths from a source to destination. The third dimension represent the correponding
	 * values for the path.
	 * delay_load_matrix[i][][][][] i represent edge switch with respect to the current edge switch i.e.
	 * the current edge switch is 0.
	 * delay_load_matrix[i][a][b][j][k] a is the index of the source host and b is the index of the destination host.
	 * delay_load_matrix[i][a][b][j][k] j is the path, k represent the corresponding value for the metric under consideration.
	 * For all i, a, b & j = 0 & k = 0: value 0 represent the flow is under threshold
	 * 							  value 1 represent flow has exceeded threshold
	 * For all i, a, b & j = 1 & k = 0: value represent the number of routes which are found. If it is 0 then it
	 * 							  means no routes have been found yet. If it has any other positive value
	 * 							  then those many routes have been found.
	 * For all i, a, b & j = 2 & k = 0: value represent the number of packets from which the load value has been
	 * 							  received. If it is 9999 then it means the load value hasn't been obtained from
	 * 							  any of the other edge switches. The number of switches from which it would
	 * 							  obtain the load packets are number of core switches + number of aggregate
	 *                            switches present in a pod in source + number of aggregate switches present
	 *                            in a pod in the destination.
	 * For all i, j, k = 1: Delay
	 * 				 k = 2: Aggregate switch along upward direction
	 * 				 k = 3: Core switch along upward direction
	 * 				 k = 4: Aggregate switch along downward direction
	 *               k = 5: Edge switch along downward direction
	 * 				 k = 6: Number of packets which can be routed along this path
	 * 				 k = 7: Load along this path
	 * 				 k = 8: Weight assigned
	 * 				 k = 9: If it's value is 1 then it indicates that the hash routing used this
	 * 						corresponding path, else it's value would be 0
	 */
	float *****delay_load_matrix;

	bool flag;

protected:
	// utility: look up interface from getArrivalGate()
	virtual InterfaceEntry *getSourceInterfaceFrom(cPacket *msg);

	// utility: show current statistics above the icon
	virtual void updateDisplayString();

	/**
	 * Encapsulate packet coming from higher layers into IPDatagram, using
	 * the control info attached to the packet.
	 */
	virtual IPDatagram *encapsulate(cPacket *transportPacket,
			InterfaceEntry *&destIE);

	/**
	 * Encapsulate packet coming from higher layers into IPDatagram, using
	 * the given control info. Override if you subclassed controlInfo and/or
	 * want to add options etc to the datagram.
	 */
	virtual IPDatagram *encapsulate(cPacket *transportPacket,
			InterfaceEntry *&destIE, IPControlInfo *controlInfo);

	/**
	 * Creates a blank IP datagram. Override when subclassing IPDatagram is needed
	 */
	virtual IPDatagram *createIPDatagram(const char *name);

	/**
	 * Handle IPDatagram messages arriving from lower layer.
	 * Decrements TTL, then invokes routePacket().
	 */
	virtual void handlePacketFromNetwork(IPDatagram *datagram);

	/**
	 * Handle messages (typically packets to be send in IP) from transport or ICMP.
	 * Invokes encapsulate(), then routePacket().
	 */
	virtual void handleMessageFromHL(cPacket *msg);

	/**
	 * Handle incoming ARP packets by sending them over "queueOut" to ARP.
	 */
	virtual void handleARP(ARPPacket *msg);

	/**
	 * Handle incoming ICMP messages.
	 */
	virtual void handleReceivedICMP(ICMPMessage *msg);

	/**
	 * Performs routing. Based on the routing decision, it dispatches to
	 * reassembleAndDeliver() for local packets, to fragmentAndSend() for forwarded packets,
	 * to handleMulticastPacket() for multicast packets, or drops the packet if
	 * it's unroutable or forwarding is off.
	 */
	virtual void routePacket(IPDatagram *datagram, InterfaceEntry *destIE, bool fromHL);

	/**
	 * Forwards packets to all multicast destinations, using fragmentAndSend().
	 */
	virtual void routeMulticastPacket(IPDatagram *datagram,
			InterfaceEntry *destIE, InterfaceEntry *fromIE);

	/**
	 * Perform reassembly of fragmented datagrams, then send them up to the
	 * higher layers using sendToHL().
	 */
	virtual void reassembleAndDeliver(IPDatagram *datagram);

	/**
	 * Decapsulate and return encapsulated packet after attaching IPControlInfo.
	 */
	virtual cPacket *decapsulateIP(IPDatagram *datagram);

	/**
	 * Fragment packet if needed, then send it to the selected interface using
	 * sendDatagramToOutput().
	 */
	virtual void fragmentAndSend(IPDatagram *datagram, InterfaceEntry *ie,
			IPAddress nextHopAddr);

	/**
	 * Last TTL check, then send datagram on the given interface.
	 */
	virtual void sendDatagramToOutput(IPDatagram *datagram, InterfaceEntry *ie,
			IPAddress nextHopAddr);

	/*
	 * This function is used to count the number of packets present in the queue corresponding
	 * to the source and destination addresses and port numbers.
	 */
	virtual int count_pkt_in_queue(int src, int dest, int srcPort, int destPort);

	/*
	 * This function fills the delay load matrix with appropriate values.
	 */
	virtual void fill_delay_load_matrix(int dest_indx, int host_indx, int intrfce_count, int src_host_indx, int dest_host_indx,int packet_count);

	/*
	 * This function sends a probe packet which is routed through the network until the destination
	 * is reached. Upon reaching the destination it is sent back to the sender. This is mainly used
	 * to identify the path information.
	 */
	virtual IPDatagram *send_probe_packet(IPAddress dest_host, IPAddress src_edge, int src_host_indx, int dest_host_indx);

	/*
	 * Fill the routes in the matrix
	 */
	virtual void fill_routes_in_delay_load_matrix(int dest_indx, IPAddress aggrg_address, IPAddress core_address, IPAddress aggrg_address_downward, IPAddress edge_address_downward, int num_intrfce, int src_host_indx, int dest_host_indx);

	/*
	 * This function sends the load estimation packet to the controller from the edge switch.
	 */
	virtual void send_load_estimation_packet(IPAddress srcAddr,IPAddress destAddr,int src_host_indx, int dest_host_indx);

	/*
	 * Here the load packet is routed from the controller or the other switches which received the packets.
	 */
	virtual void route_load_packet(IPDatagram *datagram, InterfaceEntry *destIE, bool fromHL);

	/*
	 * This function will contains the code for sending the packet which would contain the future count of the packets
	 * which would be routed along the path. Say if the path is A-B-C-D then the future count packet will start from A
	 * reach B and update there. When the a packet for determining the load arrives at B, the current queue length is
	 * returned along with the future count packet.
	 */
	virtual void send_future_packet_count(IPDatagram *base_datagram, IPAddress srcAddr, IPAddress destAddr, int relative_destination_edge_index, int src_host_indx, int dest_host_indx, int num_routes_for_this_packet);

	/*
	 * This is for routing around the failed link
	 */
	virtual void route_link_failure_packet(IPDatagram *datagram, InterfaceEntry *destIE, bool fromHL);

	/*
	 * This function is for adjusting the left out packets among the other routes
	 */
	virtual void adjust_packet_count(int c, int d, int e, int row, int num_routes_found);

	/*
	 * Called while exiting.
	 */
	virtual void finish();

public:
	IP() {
	}

protected:
	/**
	 * Initialization
	 */
	virtual void initialize();

	/**
	 * Processing of IP datagrams. Called when a datagram reaches the front
	 * of the queue.
	 */
	virtual void endService(cPacket *msg);
};

#endif

