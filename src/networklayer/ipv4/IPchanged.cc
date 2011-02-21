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


#include <omnetpp.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "IP.h"
#include "IPDatagram.h"
#include "IPControlInfo.h"
#include "ICMPMessage_m.h"
#include "IPv4InterfaceData.h"
#include "ARPPacket_m.h"
#include "IPAddress.h"


#define NUM_PACKETS 1000
#define MAX_QUEUE_SIZE 1000
#define PERCENTAGE_OF_PACKET 0.1

Define_Module(IP);

void IP::initialize(){
    QueueBase::initialize();

    ift = InterfaceTableAccess().get();
    rt = RoutingTableAccess().get();
    rt1 = RoutingTableAccess1().get();

    queueOutGate = gate("queueOut"); //ARP

    defaultTimeToLive = par("timeToLive");
    defaultMCTimeToLive = par("multicastTimeToLive");
    fragmentTimeoutTime = par("fragmentTimeout");
    mapping.parseProtocolMapping(par("protocolMapping"));

    curFragmentId = 0;
    lastCheckTime = 0;
    counter_for_interface_table = 0;
    fragbuf.init(icmpAccess.get());

    numMulticast = numLocalDeliver = numDropped = numUnroutable = 0;
    numForwarded_hash_routing_packet = numForwarded_dynamic_routing_packet = numForwarded_direct_routing_packet = 0;
    numForwarded_load_packet = numSent_load_packet = numReceived_load_packet = 0;
    numForwarded_probe_packet = numSent_probe_packet = numReceived_probe_packet = 0;
    future_packets_count = 0;

    flag = true;

    WATCH(numMulticast);
    WATCH(numLocalDeliver);
    WATCH(numDropped);
    WATCH(numUnroutable);
    WATCH(numForwarded_hash_routing_packet);
    WATCH(numForwarded_dynamic_routing_packet);
    WATCH(delay_load_matrix);
    WATCH(future_packets_count);
}

void IP::updateDisplayString()
{
    char buf[80] = "";
    sprintf(buf+strlen(buf), " future packets:%d ", future_packets_count);
    if (numLocalDeliver>0) sprintf(buf+strlen(buf), "up:%d ", numLocalDeliver);
    if (numMulticast>0) sprintf(buf+strlen(buf), "mcast:%d ", numMulticast);
    if (numDropped>0) sprintf(buf+strlen(buf), "DROP:%d ", numDropped);
    if (numUnroutable>0) sprintf(buf+strlen(buf), "UNROUTABLE:%d ", numUnroutable);
    getDisplayString().setTagArg("t",0,buf);
}

void IP::endService(cPacket *msg)
{
    if (msg->getArrivalGate()->isName("transportIn") || msg->getArrivalGate()->isName("udpIn"))
    {
        handleMessageFromHL( msg );
    }
    else if (dynamic_cast<ARPPacket *>(msg))
    {
        // dispatch ARP packets to ARP
        handleARP((ARPPacket *)msg);
    }
    else
    {
    	EV << "Handling packet from network in endservice msg" << "\n";
        IPDatagram *dgram = check_and_cast<IPDatagram *>(msg);
        handlePacketFromNetwork(dgram);
    }

    if (ev.isGUI())
        updateDisplayString();
}

InterfaceEntry *IP::getSourceInterfaceFrom(cPacket *msg)
{
    cGate *g = msg->getArrivalGate();
    return g ? ift->getInterfaceByNetworkLayerGateIndex(g->getIndex()) : NULL;
}

void IP::handlePacketFromNetwork(IPDatagram *datagram)
{
    //
    // "Prerouting"
    //
	/*
	 * The following lines are used to record the route the packet takes.
	 */
	IPRecordRouteOption r = datagram->getRecordRoute();
	//EV << "The next address pointer is " << r.getNextAddressPtr() << "\n";
	EV << "Handling packet from the network" << "\n";
	short ra = r.getNextAddressPtr();
    if(ra < 6)
    {
		r.setRecordAddress(ra,rt->getRouterId());
		ra++;
		r.setNextAddressPtr(ra);
		datagram->setRecordRoute(r);
    }

    // check for header biterror
    if (datagram->hasBitError())
    {
        // probability of bit error in header = size of header / size of total message
        // (ignore bit error if in payload)
        double relativeHeaderLength = datagram->getHeaderLength() / (double)datagram->getByteLength();
        if (dblrand() <= relativeHeaderLength)
        {
            //EV << "bit error found, sending ICMP_PARAMETER_PROBLEM\n";
            icmpAccess.get()->sendErrorMessage(datagram, ICMP_PARAMETER_PROBLEM, 0);
            return;
        }
    }

    // hop counter decrement; FIXME but not if it will be locally delivered
    datagram->setTimeToLive(datagram->getTimeToLive()-1);

    // route packet
    if (!datagram->getDestAddress().isMulticast())
    {
    	IPAddress a = rt->getRouterId();
    	//EV << "Values are assigned the switch id is" << a.getDByte(1) << "\n";

    	//Check if this node is a edge switch if so then check if a packet is arriving for the first time
    	//then initialise the edge switch's delay_load_matrix
    	if( a.getDByte(1) == 1 && flag == true)
    	{
    		//EV << "Print packett" << "\n";
    		int num_intrfce = ift->getNumInterfaces() - 2;
    		delay_load_matrix = new float****[num_intrfce * 2];
    		for(int c = 0; c < num_intrfce * 2; c++)
    		{
    			*(delay_load_matrix + c) = new float***[num_intrfce / 2];
    			for(int d = 0; d < num_intrfce / 2; d++)
    			{
    				*(*(delay_load_matrix + c) + d) = new float**[num_intrfce / 2];
    				for(int e = 0;e < num_intrfce / 2; e++)
					{
						*(*(*(delay_load_matrix + c) + d) + e) = new float*[num_intrfce];
						for(int f = 0; f < num_intrfce; f++)
						{
							*(*(*(*(delay_load_matrix + c) + d) + e)+ f) = new float[9]; // This is the final number of columns which contain the actual values
						}
						delay_load_matrix[c][d][e][0][0] = 0;
						delay_load_matrix[c][d][e][1][0] = 0;
						delay_load_matrix[c][d][e][2][0] = -1;


						delay_load_matrix[c][d][e][0][6] = 0;
						delay_load_matrix[c][d][e][1][6] = 0;
						delay_load_matrix[c][d][e][2][6] = 0;
						delay_load_matrix[c][d][e][3][6] = 0;

						delay_load_matrix[c][d][e][0][7] = 0;
						delay_load_matrix[c][d][e][1][7] = 0;
						delay_load_matrix[c][d][e][2][7] = 0;
						delay_load_matrix[c][d][e][3][7] = 0;

						delay_load_matrix[c][d][e][0][8] = 0;
						delay_load_matrix[c][d][e][1][8] = 0;
						delay_load_matrix[c][d][e][2][8] = 0;
						delay_load_matrix[c][d][e][3][8] = 0;
					}
    			}
    		}

    		numForwarded_per_host = new int*[num_intrfce / 2];

    		for(int c = 0; c < num_intrfce / 2; c++)
    			*(numForwarded_per_host + c) = new int[(num_intrfce * num_intrfce * num_intrfce) / 4];


    		for(int c = 0 ; c < num_intrfce / 2 ; c++)
    			for(int c1 = 0 ; c1 < ((num_intrfce * num_intrfce * num_intrfce) / 4) ; c1++)
    				numForwarded_per_host[c][c1] = 0;
        	flag = false;
    	}

    	if(datagram->getTransportProtocol() == 1 || datagram->getTransportProtocol() == 17)
    	{
    		routePacket(datagram, NULL, false);
    	}
    	else if(datagram->getTransportProtocol() == 46)
    	{
    		route_load_packet(datagram, NULL, false);
    	}
    	else if(datagram->getTransportProtocol() == 2)
    	{
    		route_link_failure_packet(datagram,NULL,false);
    	}
    }
    else
        routeMulticastPacket(datagram, NULL, getSourceInterfaceFrom(datagram));
}

void IP::handleARP(ARPPacket *msg)
{
    // FIXME hasBitError() check  missing!

    // delete old control info
    delete msg->removeControlInfo();

    // dispatch ARP packets to ARP and let it know the gate index it arrived on
    InterfaceEntry *fromIE = getSourceInterfaceFrom(msg);
    ASSERT(fromIE);

    IPRoutingDecision *routingDecision = new IPRoutingDecision();
    routingDecision->setInterfaceId(fromIE->getInterfaceId());
    msg->setControlInfo(routingDecision);

    send(msg, queueOutGate);
}

void IP::handleReceivedICMP(ICMPMessage *msg)
{
    switch (msg->getType())
    {
        case ICMP_REDIRECT: // TODO implement redirect handling
        case ICMP_DESTINATION_UNREACHABLE:
        case ICMP_TIME_EXCEEDED:
        case ICMP_PARAMETER_PROBLEM: {
            // ICMP errors are delivered to the appropriate higher layer protocol
            IPDatagram *bogusPacket = check_and_cast<IPDatagram *>(msg->getEncapsulatedMsg());
            int protocol = bogusPacket->getTransportProtocol();
            int gateindex = mapping.getOutputGateForProtocol(protocol);
            send(msg, "transportOut", gateindex);
            break;
        }
        default: {
            // all others are delivered to ICMP: ICMP_ECHO_REQUEST, ICMP_ECHO_REPLY,
            // ICMP_TIMESTAMP_REQUEST, ICMP_TIMESTAMP_REPLY, etc.
            int gateindex = mapping.getOutputGateForProtocol(IP_PROT_ICMP);
            send(msg, "transportOut", gateindex);
        }
    }
}

void IP::handleMessageFromHL(cPacket *msg)
{
    // if no interface exists, do not send datagram
    if (ift->getNumInterfaces() == 0)
    {
        //EV << "No interfaces exist, dropping packet\n";
        delete msg;
        return;
    }

    // encapsulate and send
    InterfaceEntry *destIE; // will be filled in by encapsulate()
    IPDatagram *datagram = encapsulate(msg, destIE);

    // route packet
    if (!datagram->getDestAddress().isMulticast())
        routePacket(datagram, destIE, true);
    else
        routeMulticastPacket(datagram, destIE, NULL);
}

void IP::routePacket(IPDatagram *datagram, InterfaceEntry *destIE, bool fromHL)
{
    // TBD add option handling code here
    IPAddress destAddr = datagram->getDestAddress();
    IPAddress srcAddr = datagram->getSrcAddress();
    IPAddress this_node_address = rt->getRouterId();

	int dest = destAddr.getInt();
	int src = srcAddr.getInt();
	int destPort = datagram->getDestPort();
	int srcPort = datagram->getSrcPort();
    IPAddress nextHopAddr;

	int counter;
	float min_delay = 9999;
	int path_to_be_taken;

	/*
	 * These two variables contain the output interface through which the packet will be routed
	 * during upstream
	 */
	uint32 unconverted_core_address;
	uint32 unconverted_aggregate_address;

	/*
	 *  This variable contains the value of the destination edge switch's switch id.
	 */
	int dest_switch_id = destAddr.getDByte(2);

	/*
	 * Switch id of this edge switch
	 */
	int srce_switch_id =  srcAddr.getDByte(2);

	int index_difference = dest_switch_id - srce_switch_id;
	int num_interface_for_this_node = ift->getNumInterfaces() - 2;

	int num_packets_obtained_for_load_estimation;

	int num_routes_for_this_packet;
	/*
	 * This variable contains the destination edge switches id with respect to the current edge
	 * switch. The current edge switch is given value 0.
	 *
	 */
	int relative_destination_edge_index = (index_difference > 0) ? index_difference: (index_difference + ((num_interface_for_this_node * num_interface_for_this_node) / 2));

	int layer_id_of_this_node = this_node_address.getDByte(1);
	int get_count_of_number_of_packets_in_queue;

	/*
	 * src_host_indx represent source host which is connected to the edge switch.
	 * Host1 will have indx value 0.
	 */
	int src_host_indx = srcAddr.getDByte(3) - 1;

	/*
	 * dest_host_indx represent destination host which is connected to the edge switch.
	 */
	int dest_host_indx = destAddr.getDByte(3) - 1;


	/*
	 * relative_destination_host_index represent the destination host with respect to the source host.
	 */
	int relative_destination_host_index = relative_destination_edge_index * (num_interface_for_this_node / 2) + destAddr.getDByte(3) - srcAddr.getDByte(3);

    EV << "Routing datagram `" << datagram->getName() << "' with dest=" << destAddr << ", src=" << srcAddr << "\n";

    if(srcAddr.getDByte(0) == destAddr.getDByte(0))
    	num_packets_obtained_for_load_estimation = num_routes_for_this_packet = num_interface_for_this_node / 2;
    else
    {
    	num_packets_obtained_for_load_estimation = num_interface_for_this_node + (num_interface_for_this_node * num_interface_for_this_node) / 4;
    	num_routes_for_this_packet = (num_interface_for_this_node * num_interface_for_this_node) / 4;
    }

    if(datagram->getTransportProtocol() != 1)
	{
		// check for local delivery
		if (rt->isLocalAddress(destAddr))
		{
			EV << "local delivery\n";
			if (datagram->getSrcAddress().isUnspecified())
				datagram->setSrcAddress(destAddr); // allows two apps on the same host to communicate
			numLocalDeliver++;
			reassembleAndDeliver(datagram);
			return;
		}

		// if datagram arrived from input gate and IP_FORWARD is off, delete datagram
		if (!fromHL && !rt->isIPForwardingEnabled())
		{
			EV << "forwarding off, dropping packet\n";
			numDropped++;
			delete datagram;
			return;
		}

		delete datagram->removeControlInfo();

		// if output port was explicitly requested, use that, otherwise use IP routing
		if (destIE)
		{
			EV << "using manually specified output interface " << destIE->getName() << "\n";
			// and nextHopAddr remains unspecified
		}
		else
		{
			const IPRoute *re = NULL;
			// check primary routing table for prefix match 1st
			EV << "About to check for best matching route with " << destAddr <<"\n";
			re = rt->findBestMatchingRoute(destAddr);
			EV << "The return value from re is " << re <<"\n";
			// if no prefix match, carry on with multipath selection
			if(re != NULL)
			{
				numForwarded_direct_routing_packet++;

				IPRecordRouteOption r = datagram->getRecordRoute();
				if(datagram->getOutput_port_to_core_switch() == 99 && layer_id_of_this_node == 2 && r.getNextAddressPtr() <= 3)
				{
					InterfaceEntry *destiIE;
					IPRecordRouteOption r1;
					IPAddress nextHopAddrr;
					IPAddress nul(0,0,0,1);
					IPAddress edge_from_where_the_packet_arrived = r.getRecordAddress(r.getNextAddressPtr() - 2);
					EV << "The edge from which the datagram came is sent is " << edge_from_where_the_packet_arrived << "\n";
					IPDatagram *probe_datagram = send_probe_packet(edge_from_where_the_packet_arrived,this_node_address,src_host_indx,dest_host_indx);
					EV << "The probe packet to the edge is sent by the aggregate switch" << "\n";
					probe_datagram->setArrival(datagram->getArrivalModule(),datagram->getArrivalGateId());
					//probe_datagram->setArrivalTime(probe_datagram->getCreationTime()+0.0000000000001);
					//probe_datagram->setTimeToLive(probe_datagram->getTimeToLive() - 1);
					probe_datagram->setPacketId(1);
					probe_datagram->setFlowId(relative_destination_edge_index);
					probe_datagram->setLink_status(0);
					int c = 0;
					r1.setRecordAddress(c++,edge_from_where_the_packet_arrived);
					r1.setNextAddressPtr(c);
					probe_datagram->setRecordRoute(r1);
					r1.setRecordAddress(c++,edge_from_where_the_packet_arrived);
					r1.setNextAddressPtr(c);
					probe_datagram->setRecordRoute(r1);
					r1.setRecordAddress(c++,r.getRecordAddress(r.getNextAddressPtr() - 1));
					r1.setNextAddressPtr(c);
					probe_datagram->setRecordRoute(r1);
					r1.setRecordAddress(c++,re->getHost());
					r1.setNextAddressPtr(c);
					probe_datagram->setRecordRoute(r1);
					r1.setRecordAddress(c++,nul);
					r1.setNextAddressPtr(c);
					probe_datagram->setRecordRoute(r1);
					r1.setRecordAddress(c++,nul);
					r1.setNextAddressPtr(c);
					probe_datagram->setRecordRoute(r1);

					for(int c = 0; c < rt->getNumRoutes(); c++)
					{
						const IPRoute *rty = rt->getRoute(c);
						if(edge_from_where_the_packet_arrived.equals(rty->getHost()))
						{
							destiIE = rty->getInterface();
							nextHopAddrr = rty->getGateway();
						}
					}
					fragmentAndSend(probe_datagram, destiIE, nextHopAddrr);
					numForwarded_probe_packet++;
				}
			}
			else if (re==NULL)
			{
				EV << "It has come into this for checking" << "\n";

				if (layer_id_of_this_node == 1)
				{
					/*
					 * If the number of packets sent from this host to destination is 0 then along with
					 * original datagram routed through hash routing, send a probe packet to identify the route.
					 */
					if(numForwarded_per_host[src_host_indx][relative_destination_host_index] == 0)
					{
						IPDatagram *probe_datagram = send_probe_packet(destAddr,this_node_address,src_host_indx,dest_host_indx);
						numSent_probe_packet++;
						EV << "First packet sent from the host " << datagram->getSrcAddress() << " to " << datagram->getDestAddress() << "\n";
						EV << "The arrival gate for this datagram is " << datagram->getArrivalGate() << "\n";
						EV << "The arrival gate id for this datagram is " << datagram->getArrivalGateId() << "\n";
						EV << "The arrival module for this datagram is " << datagram->getArrivalModule() << "\n";
						EV << "The arrival module id for this datagram is " << datagram->getArrivalModuleId() << "\n";
						EV << "The arrival time for this datagram is " << datagram->getArrivalTime() << "\n";

						EV << "The probe packet created is sent from " << probe_datagram->getSrcAddress() << " to " << probe_datagram->getDestAddress() << "\n";
						EV << "The protocol is " << probe_datagram->getTransportProtocol() << "\n";
						probe_datagram->setArrival(datagram->getArrivalModule(),datagram->getArrivalGateId());
						// Just insert one probe datagram into the pqueue and then this would be duplicated later on.
						pqueue.insert(probe_datagram);
						for(int c = 0; c < num_routes_for_this_packet; c++)
						{
							delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][c][1] = SIMTIME_DBL(simulation.getSimTime());
						}
					}

					/**
					 * The first row and first column in delay_load_matrix is used to represent if the
					 * threshold for this flow has exceeded. If the value is 0 then it hasn't exceeded
					 * and the count of the number of packets corresponding to the source addr, destination
					 * addr, source port and destination port is obtained and hash routing is used for this
					 * flow, if the value is 1 then we won't need the count any more and we use dynamic routing.
					 */
					EV << "The delay load matrix value is " << delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][0][0] << "\n";
					if(delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][0][0] == 0)
					{
						get_count_of_number_of_packets_in_queue = count_pkt_in_queue(src,dest,srcPort,destPort);
						EV << "The number of packets in queue for this src " << src << ", dest " << dest << " is " << get_count_of_number_of_packets_in_queue << "\n";
					}

					if(delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][0][0] == 0 && get_count_of_number_of_packets_in_queue < PERCENTAGE_OF_PACKET * MAX_QUEUE_SIZE)
					{
						datagram->setOutput_port_to_core_switch(99);
						re = rt1->hashRouting(dest, src, destPort, srcPort);
						numForwarded_hash_routing_packet++;
					}
					else
					{
						EV << "The threshold reached" << "\n";
						if(delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][2][0] == -1 && delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][1][0] == num_routes_for_this_packet)
						{
							EV << "Estimating load here" << "\n";
							send_load_estimation_packet(srcAddr,destAddr,src_host_indx,dest_host_indx);
							numSent_load_packet++;
							delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][2][0] = 0;
						}
						// Check if all the routes are obtained if so then perform dynamic routing else use hash
						// routing itself.
						EV << "The value in matrix for num routes is " << delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][1][0] << "\n";
						EV << "The value in matrix for num packets received is " << delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][2][0] << "\n";
						EV << "The total routes is " << num_routes_for_this_packet << "\n";
						EV << "The total packets is " << num_packets_obtained_for_load_estimation << "\n";
						if(delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][1][0] == num_routes_for_this_packet && delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][2][0] == num_packets_obtained_for_load_estimation && delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][0][0] == 0)
						{
							EV << "Routes and the load are successfully found" << "\n";
							fill_delay_load_matrix(relative_destination_edge_index,relative_destination_host_index,num_interface_for_this_node,src_host_indx,dest_host_indx,datagram->getPacketId());
							send_future_packet_count(datagram,srcAddr,destAddr,relative_destination_edge_index,src_host_indx,dest_host_indx,num_routes_for_this_packet);
							delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][0][0] = 1;
						}
						if(delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][1][0] < num_routes_for_this_packet || delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][2][0] < num_packets_obtained_for_load_estimation)
						{
							EV << "Routes and load aren't found for all the paths yet, hence using hash technique" << "\n";
							datagram->setOutput_port_to_core_switch(99);
							re = rt1->hashRouting(dest, src, destPort, srcPort);
							numForwarded_hash_routing_packet++;
						}
						else
						{
							// Fill the matrix only for the destined current flow, this is done only if after getting all
							// the routes for the destined output.
							// The threshold is reached for this flow and dynamic is routing is used.

							EV << "The current queue length is " << get_Length() << "\n";
							EV << "The dest switch id is " << dest_switch_id << "\n";
							EV << "The current node's switch id is " << srce_switch_id << "\n";
							EV << "The index difference is " << index_difference << "\n";
							EV << "Number of interfaces connected to this node are " << num_interface_for_this_node << "\n";
							EV << "Relative dest edge index is  " << relative_destination_edge_index << "\n";
							EV << "Has the value changed " << delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][0][0] << "\n";

							for(counter = 0;counter < num_routes_for_this_packet;counter++)
							{
								if(min_delay > delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][counter][1] && delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][counter][6] > 0)
								{
									min_delay = delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][counter][1];
									path_to_be_taken = counter;
								}
							}

							/*
							 * Reduce the packet count of the number of packets sent along the min
							 * delay path. If the value reaches zero no more packets can be sent along
							 * this path and an alternative path is chosen in the for loop above.
							 */
							unconverted_aggregate_address = delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][path_to_be_taken][2];
							unconverted_core_address = delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][path_to_be_taken][3];
							delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][path_to_be_taken][6]--;

							EV << "The path to be taken is " << path_to_be_taken << "\n";
							EV << "The delay load after decrement " << delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][path_to_be_taken][6] << "\n";

							for(int c = 0; c < num_routes_for_this_packet; c++)
								EV << "The path" << c << " 's packet left out is " << delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][c][6] << "\n";

							//The identified output gate from the matrix is attached with the string ppp
							EV << "The min delay is  " << min_delay << "\n";
							EV << "The output gate for edge switch is  " << unconverted_aggregate_address << "\n";
							EV << "The output gate for aggregate switch is " << unconverted_core_address << "\n";

							//The datagram is modified to contain the next output gate from the aggregate switch
							datagram->setOutput_port_to_core_switch(unconverted_core_address);
							EV << " The datagram has been changed " << "\n";
							IPAddress ipad(unconverted_aggregate_address);
							/*
							 * Get the corresponding route index in the routing table based on a match between the string
							 * generated and the one in the routing table 1.
							 */

							for(counter = 0;counter < rt1->getNumRoutes();counter++)
							{
								const IPRoute *rty = rt1->getRoute(counter);
								if(ipad.equals(rty->getHost()))
									break;
							}

							EV << "The counter value is " << counter << "\n";

							//Increment the count for the host which sends the data
							re = rt1->getRoute(counter);
							numForwarded_dynamic_routing_packet++;
						}
					}
					numForwarded_per_host[src_host_indx][relative_destination_host_index]++;
					EV << "The packets forwarded for this host " << src_host_indx + 1 << " is " << numForwarded_per_host[src_host_indx][relative_destination_host_index] << "\n";
				}
				else if (layer_id_of_this_node == 2)
				{
					/*
					 * We have obtained the core switches ip address. The next hop should be to this
					 * address. However it's value is in uint32 which has to be converted to IPAddress
					 * format and the so obtained IPAddress is matched with the routing table 1's IPAddresses.
					 * The interface of the corresponding matched IPAddress is used for the next hop.
					 */
					if(datagram->getOutput_port_to_core_switch() == 99)
					{
						InterfaceEntry *destiIE;
						re = rt1->hashRouting(dest, src, destPort, srcPort);
						IPRecordRouteOption r = datagram->getRecordRoute();
						IPRecordRouteOption r1;
						IPAddress nextHopAddrr;
						IPAddress nul(0,0,0,1);
						IPAddress edge_from_where_the_packet_arrived = r.getRecordAddress(r.getNextAddressPtr() - 2);
						EV << "The edge from which the datagram came is sent is " << edge_from_where_the_packet_arrived << "\n";
						IPDatagram *probe_datagram = send_probe_packet(edge_from_where_the_packet_arrived,this_node_address,src_host_indx,dest_host_indx);
						EV << "The probe packet to the edge is sent by the aggregate switch" << "\n";
						probe_datagram->setArrival(datagram->getArrivalModule(),datagram->getArrivalGateId());
						probe_datagram->setPacketId(1);
						probe_datagram->setFlowId(relative_destination_edge_index);
						probe_datagram->setLink_status(0);
						int c = 0;
						r1.setRecordAddress(c++,edge_from_where_the_packet_arrived);
						r1.setNextAddressPtr(c);
						probe_datagram->setRecordRoute(r1);
						r1.setRecordAddress(c++,edge_from_where_the_packet_arrived);
						r1.setNextAddressPtr(c);
						probe_datagram->setRecordRoute(r1);
						r1.setRecordAddress(c++,r.getRecordAddress(r.getNextAddressPtr() - 1));
						r1.setNextAddressPtr(c);
						probe_datagram->setRecordRoute(r1);
						r1.setRecordAddress(c++,re->getHost());
						r1.setNextAddressPtr(c);
						probe_datagram->setRecordRoute(r1);
						r1.setRecordAddress(c++,nul);
						r1.setNextAddressPtr(c);
						probe_datagram->setRecordRoute(r1);
						r1.setRecordAddress(c++,nul);
						r1.setNextAddressPtr(c);
						probe_datagram->setRecordRoute(r1);

						for(int c = 0; c < rt->getNumRoutes(); c++)
						{
							const IPRoute *rty = rt->getRoute(c);
							if(edge_from_where_the_packet_arrived.equals(rty->getHost()))
							{
								destiIE = rty->getInterface();
								nextHopAddrr = rty->getGateway();
							}
						}
						fragmentAndSend(probe_datagram, destiIE, nextHopAddrr);
						numForwarded_probe_packet++;
						numForwarded_hash_routing_packet++;
					}
					else
					{
						unconverted_core_address = datagram->getOutput_port_to_core_switch();
						EV << "Output route to be taken is to the core switch " << unconverted_core_address << "\n";
						IPAddress ipad(unconverted_core_address);

						for(counter = 0;counter < rt1->getNumRoutes();counter++)
						{
							const IPRoute *rty = rt1->getRoute(counter);
							if(ipad.equals(rty->getHost()))
								break;
						}
						re = rt1->getRoute(counter);
						numForwarded_dynamic_routing_packet++;
					}
				}
			}
			// error handling: destination address does not exist in routing table:
			// notify ICMP, throw packet away and continue
			if (re==NULL)
			{
				EV << "unroutable, sending ICMP_DESTINATION_UNREACHABLE\n";
				numUnroutable++;
				icmpAccess.get()->sendErrorMessage(datagram, ICMP_DESTINATION_UNREACHABLE, 0);
				return;
			}

			// extract interface and next-hop address from routing table entry
			destIE = re->getInterface();
			nextHopAddr = re->getGateway();
		}
		if (datagram->getSrcAddress().isUnspecified())
			datagram->setSrcAddress(destIE->ipv4Data()->getIPAddress());

		if(datagram->getOutput_port_to_core_switch() != 99 && layer_id_of_this_node > 1)
			future_packets_count--;
		// fragment and send the packet
		fragmentAndSend(datagram, destIE, nextHopAddr);
	}
    else if(datagram->getTransportProtocol() == 1)
    {
    	if(datagram->getNum_packets_for_this_path() == -1)
    	{
    		/*
    		 * Num_packet_for_this_path contains the number of future packets which will be
    		 * routed along this path. If it's value is -1 then it means that the packet which
    		 * has arrived is determining the path and not setting the future packet count.
    		 */
			const IPRoute *probe_route = NULL;
			/*
			 * It has come into this section of route packet meaning the packet under consideration is
			 * a packet which is used to identify the routes.
			 * The packet is routed from the edge connected to the source host to the edge connected to
			 * the destination host. In the forward direction the packet carries a packet id of 0 and in the
			 * reverse direction the packet id is set to 1(just for identification in which direction the packet
			 * is currently traversing.
			 * The probe packet records the route taken using IPRecordrouteoption. When a probe packet reaches
			 * a switch the packet is duplicated and sent along the switches output port. If the packet id is
			 * 0 then it is upstream travel and hence packet is sent to all output ports which are connected to
			 * next higher level switches i.e. if current switch is aggregate switch, then the duplicated packets
			 * are sent to all core switches connected to this aggregate switch.
			 * In the downstream direction the path is deterministic and hence the function findBestMatchingRoute
			 * is used to find the best path to take.
			 */
			if(!destAddr.equals(rt->getRouterId()) && datagram->getPacketId() == 0)
			{
				/*
				 * This is for the forward path from source edge switch to destination edge switch.
				 */
				EV << "The outcome of this check is " << destAddr.equals(rt->getRouterId()) << "\n";
				probe_route = rt->findBestMatchingRoute(datagram->getDestAddress());
				if(probe_route == NULL)
				{
						while(counter_for_interface_table != rt1->getNumRoutes() - 1)
						{
							probe_route = rt1->getRoute(counter_for_interface_table);
							counter_for_interface_table++;
							EV << "The current value is " << counter_for_interface_table << "\n";
							destIE = probe_route->getInterface();
							nextHopAddr = probe_route->getGateway();
							IPDatagram *d = datagram->dup();
							fragmentAndSend(d, destIE, nextHopAddr);
						}

						probe_route = rt1->getRoute(counter_for_interface_table);
						counter_for_interface_table++;
						EV << "The current value is " << counter_for_interface_table << "\n";

						if(counter_for_interface_table == rt1->getNumRoutes())
							counter_for_interface_table = 0;
				}
				destIE = probe_route->getInterface();
				nextHopAddr = probe_route->getGateway();
				fragmentAndSend(datagram, destIE, nextHopAddr);
			}
			else if(destAddr.equals(rt->getRouterId()) | datagram->getPacketId() == 1)
			{
				/*
				 * This is for the reverse path from destination edge switch to source edge switch. And
				 * hence the recorded routes are used to trace the path. The recorded route is matched
				 * with both the routing tables to find the next output interface.
				 */
				datagram->setPacketId(1);
				IPRecordRouteOption r = datagram->getRecordRoute();
				bool foundd = false;
				int k = 0;
				IPAddress ipa = r.getRecordAddress(k);
				EV << "The address obtained is " << ipa << "\n";
				EV << "The address obtained is " << r.getRecordAddress(k) << "\n";
				while(!ipa.equals(rt->getRouterId()))
				{
					k++;
					ipa = r.getRecordAddress(k);
				}
				if(k > 0)
				{
					// This is for re-routing the packet along the path from which it came.
					EV << "The k value is " << k << "\n";
					k--;
					ipa = r.getRecordAddress(k);
					// Look for a match in the rt table. RT table contains the downstream interface routes.
					for(int c = 0;c < rt->getNumRoutes();c++)
					{
						probe_route = rt->getRoute(c);
						if(ipa.equals(probe_route->getHost()))
						{
							foundd = true;
							break;
						}
					}
					// If not found then look for it in the rt1 table. RT1 table contains the upstream routes.
					if(foundd != true)
					{
						for(int c = 0;c < rt1->getNumRoutes();c++)
						{
							probe_route = rt1->getRoute(c);
							if(ipa.equals(probe_route->getHost()))
							{
								foundd = true;
								break;
							}
						}
					}
//					numForwarded_probe_packet++;
					destIE = probe_route->getInterface();
					nextHopAddr = probe_route->getGateway();
					fragmentAndSend(datagram, destIE, nextHopAddr);
				}
				else
				{
					/* This means the packet which originated from the source has reached the source
					 * again. Hence the route is determined and it needs to be recorded.*/

					EV << "Else it has come into this " << "\n";
					IPAddress aggregate_address = r.getRecordAddress(2);
					IPAddress core_address = r.getRecordAddress(3);
					IPAddress aggregate_address_downward = r.getRecordAddress(4);
					IPAddress edge_address_downward = r.getRecordAddress(5);
					int src_host_index = datagram->getSrcPort();
					int dest_host_index = datagram->getDestPort();
					uint32 core_add = core_address.getInt();
					IPAddress ap(core_add);
					EV << "The IP is " << ap.str() << "\n";
					EV << "The aggregate is " << aggregate_address << " and core address is " << core_address << "\n";
					numReceived_probe_packet++;
					// This value is set only for the packet which is sent to determine the hash
					// routing path taken.
					if(datagram->getFlowId() != -1)
						relative_destination_edge_index = datagram->getFlowId();
					fill_routes_in_delay_load_matrix(relative_destination_edge_index,aggregate_address,core_address,aggregate_address_downward,edge_address_downward,num_interface_for_this_node,src_host_index,dest_host_index);
					delete datagram;
				}
			}
    	}
    	else
    	{
    		/*
    		 * Set the future packet count for the current switch and then forward the packet
    		 * based on the route specified in the IPRecordRoute
    		 */

			IPRecordRouteOption r = datagram->getRecordRoute();
			int k = 0;
			IPAddress ipa;
			const IPRoute *re = NULL;
			do
			{
				ipa = r.getRecordAddress(k);
				k++;
			}while(!ipa.equals(rt->getRouterId()));

			IPAddress next_hop_address = r.getRecordAddress(k);
			EV << "The next hop address to be taken is " << next_hop_address << "\n";
			if(k < datagram->getPacketId()) //PacketId contains the number of routes for the packet.
			{
				re = rt->findBestMatchingRoute(next_hop_address);
				if(re == NULL)
				{
					for(int c = 0; c < rt1->getNumRoutes(); c++)
					{
						re = rt1->getRoute(c);
						if(next_hop_address.equals(re->getHost()))
							break;
					}
				}
				EV << "The future packet count is " << future_packets_count << "\n";
				future_packets_count = future_packets_count + datagram->getNum_packets_for_this_path();
				destIE = re->getInterface();
				nextHopAddr = re->getGateway();
				fragmentAndSend(datagram, destIE, nextHopAddr);
			}
			else
			{
				EV << "It has reached the aggregate at the destination and deleting the packet" << "\n";
				future_packets_count = future_packets_count + datagram->getNum_packets_for_this_path();
				delete datagram;
			}
    		EV << "Packet received appropriately" << "\n";
//    		numForwarded_probe_packet++;
    	}
		numForwarded_probe_packet++;
    }
}

void IP::routeMulticastPacket(IPDatagram *datagram, InterfaceEntry *destIE, InterfaceEntry *fromIE)
{
    IPAddress destAddr = datagram->getDestAddress();
    EV << "Routing multicast datagram `" << datagram->getName() << "' with dest=" << destAddr << "\n";

    numMulticast++;

    // DVMRP: process datagram only if sent locally or arrived on the shortest
    // route (provided routing table already contains srcAddr); otherwise
    // discard and continue.
    InterfaceEntry *shortestPathIE = rt->getInterfaceForDestAddr(datagram->getSrcAddress());
    if (fromIE!=NULL && shortestPathIE!=NULL && fromIE!=shortestPathIE)
    {
        // FIXME count dropped
        EV << "Packet dropped.\n";
        delete datagram;
        return;
    }

    // if received from the network...
    if (fromIE!=NULL)
    {
        // check for local delivery
        if (rt->isLocalMulticastAddress(destAddr))
        {
            IPDatagram *datagramCopy = (IPDatagram *) datagram->dup();

            // FIXME code from the MPLS model: set packet dest address to routerId (???)
            datagramCopy->setDestAddress(rt->getRouterId());

            reassembleAndDeliver(datagramCopy);
        }

        // don't forward if IP forwarding is off
        if (!rt->isIPForwardingEnabled())
        {
            delete datagram;
            return;
        }

        // don't forward if dest address is link-scope
        if (destAddr.isLinkLocalMulticast())
        {
            delete datagram;
            return;
        }

    }

    // routed explicitly via IP_MULTICAST_IF
    if (destIE!=NULL)
    {
        ASSERT(datagram->getDestAddress().isMulticast());

        EV << "multicast packet explicitly routed via output interface " << destIE->getName() << endl;

        // set datagram source address if not yet set
        if (datagram->getSrcAddress().isUnspecified())
            datagram->setSrcAddress(destIE->ipv4Data()->getIPAddress());

        // send
        fragmentAndSend(datagram, destIE, datagram->getDestAddress());

        return;
    }

    // now: routing
    MulticastRoutes routes = rt->getMulticastRoutesFor(destAddr);
    if (routes.size()==0)
    {
        // no destination: delete datagram
        delete datagram;
    }
    else
    {
        // copy original datagram for multiple destinations
        for (unsigned int i=0; i<routes.size(); i++)
        {
            InterfaceEntry *destIE = routes[i].interf;

            // don't forward to input port
            if (destIE && destIE!=fromIE)
            {
                IPDatagram *datagramCopy = (IPDatagram *) datagram->dup();

                // set datagram source address if not yet set
                if (datagramCopy->getSrcAddress().isUnspecified())
                    datagramCopy->setSrcAddress(destIE->ipv4Data()->getIPAddress());

                // send
                IPAddress nextHopAddr = routes[i].gateway;
                fragmentAndSend(datagramCopy, destIE, nextHopAddr);
            }
        }

        // only copies sent, delete original datagram
        delete datagram;
    }
}

void IP::reassembleAndDeliver(IPDatagram *datagram)
{
    // reassemble the packet (if fragmented)
    if (datagram->getFragmentOffset()!=0 || datagram->getMoreFragments())
    {
        EV << "Datagram fragment: offset=" << datagram->getFragmentOffset() << ", MORE=" << (datagram->getMoreFragments() ? "true" : "false") << ".\n";

        // erase timed out fragments in fragmentation buffer; check every 10 seconds max
        if (simTime() >= lastCheckTime + 10)
        {
            lastCheckTime = simTime();
            fragbuf.purgeStaleFragments(simTime()-fragmentTimeoutTime);
        }

        datagram = fragbuf.addFragment(datagram, simTime());
        if (!datagram)
        {
            EV << "No complete datagram yet.\n";
            return;
        }
        EV << "This fragment completes the datagram.\n";
    }

    // decapsulate and send on appropriate output gate
    int protocol = datagram->getTransportProtocol();
    cPacket *packet = decapsulateIP(datagram);

    if (protocol==IP_PROT_ICMP)
    {
        // incoming ICMP packets are handled specially
        handleReceivedICMP(check_and_cast<ICMPMessage *>(packet));
    }
    else if (protocol==IP_PROT_IP)
    {
        // tunnelled IP packets are handled separately
        send(packet, "preRoutingOut");
    }
    else
    {
        int gateindex = mapping.getOutputGateForProtocol(protocol);
        send(packet, "transportOut", gateindex);
    }
}

cPacket *IP::decapsulateIP(IPDatagram *datagram)
{
    // decapsulate transport packet
    InterfaceEntry *fromIE = getSourceInterfaceFrom(datagram);
    cPacket *packet = datagram->decapsulate();

    // create and fill in control info
    IPControlInfo *controlInfo = new IPControlInfo();
    controlInfo->setProtocol(datagram->getTransportProtocol());
    controlInfo->setSrcAddr(datagram->getSrcAddress());
    controlInfo->setDestAddr(datagram->getDestAddress());
    controlInfo->setFlowId(datagram->getFlowId());
    controlInfo->setLink_status(datagram->getLink_status());
    controlInfo->setPacketId(datagram->getPacketId());
    controlInfo->setDiffServCodePoint(datagram->getDiffServCodePoint());
    controlInfo->setInterfaceId(fromIE ? fromIE->getInterfaceId() : -1);

    // original IP datagram might be needed in upper layers to send back ICMP error message
    controlInfo->setOrigDatagram(datagram);

    // attach control info
    packet->setControlInfo(controlInfo);

    return packet;
}


// Fixed by Vincent
void IP::fragmentAndSend(IPDatagram *datagram, InterfaceEntry *ie, IPAddress nextHopAddr)
{
    int mtu = ie->getMTU();

    // check if datagram does not require fragmentation
    if (datagram->getByteLength() <= mtu)
    {
        sendDatagramToOutput(datagram, ie, nextHopAddr);
        return;
    }

    int headerLength = datagram->getHeaderLength();
    int byteLength = datagram->getByteLength();
    int payload = byteLength - headerLength;
    float frag = (payload / (mtu - headerLength)) + 0.5;

    int noOfFragments = int(ceil(frag));

    // if "don't fragment" bit is set, throw datagram away and send ICMP error message
    if (datagram->getDontFragment() && noOfFragments>1)
    {
        EV << "datagram larger than MTU and don't fragment bit set, sending ICMP_DESTINATION_UNREACHABLE\n";
        icmpAccess.get()->sendErrorMessage(datagram, ICMP_DESTINATION_UNREACHABLE,
                                                     ICMP_FRAGMENTATION_ERROR_CODE);
        return;
    }

    // create and send fragments
    EV << "Breaking datagram into " << noOfFragments << " fragments\n";
    std::string fragMsgName = datagram->getName();
    fragMsgName += "-frag";

    for (int i=0; i<noOfFragments; i++) {
    	IPDatagram *fragment = (IPDatagram *) datagram->dup();
    	fragment->setName(fragMsgName.c_str());

    	// total_length equal to mtu, except for last fragment;
    	// "more fragments" bit is unchanged in the last fragment, otherwise true
    	if (i != noOfFragments-1) {
    		fragment->setMoreFragments(true);
    	} else {
    		fragment->setMoreFragments(false);
    	}
    	fragment->setPacketId(i);
    	fragment->setByteLength(mtu);
    	fragment->setFragmentOffset(i * (mtu - datagram->getHeaderLength()));

    	sendDatagramToOutput(fragment, ie, nextHopAddr);
    }
    delete datagram;
}


IPDatagram *IP::encapsulate(cPacket *transportPacket, InterfaceEntry *&destIE)
{
    IPControlInfo *controlInfo = check_and_cast<IPControlInfo*>(transportPacket->removeControlInfo());
    IPDatagram *datagram = encapsulate(transportPacket, destIE, controlInfo);
    delete controlInfo;
    return datagram;
}

IPDatagram *IP::encapsulate(cPacket *transportPacket, InterfaceEntry *&destIE, IPControlInfo *controlInfo)
{
	IPAddress temp = rt->getRouterId();
	char packet_name[60];
	strcpy(packet_name,temp.str().c_str());
	strcat(packet_name," ");
	strcat(packet_name,transportPacket->getName());
    IPDatagram *datagram = createIPDatagram(packet_name);
    datagram->setByteLength(IP_HEADER_BYTES);
    datagram->encapsulate(transportPacket);

    // set source and destination address
    IPAddress dest = controlInfo->getDestAddr();
    int destPort = controlInfo->getDestPort();
    datagram->setDestAddress(dest);
    datagram->setDestPort(destPort);

    // IP_MULTICAST_IF option, but allow interface selection for unicast packets as well
    destIE = ift->getInterfaceById(controlInfo->getInterfaceId());

    IPAddress src = controlInfo->getSrcAddr();

    // when source address was given, use it; otherwise it'll get the address
    // of the outgoing interface after routing
    if (!src.isUnspecified()) {
        // if interface parameter does not match existing interface, do not send datagram
        if (rt->getInterfaceByAddress(src)==NULL)
            opp_error("Wrong source address %s in (%s)%s: no interface with such address",
                      src.str().c_str(), transportPacket->getClassName(), transportPacket->getFullName());
        datagram->setSrcAddress(src);
    } else {
    	IPAddress routerId = rt->getRouterId();
    	datagram->setSrcAddress(routerId);
    }

    int srcPort = controlInfo->getSrcPort();
    datagram->setSrcPort(srcPort);
    int flow = controlInfo->getFlowId();
    datagram->setFlowId(flow);
    int pkt = controlInfo->getPacketId();
    datagram->setPacketId(pkt);
    int link_status = controlInfo->getLink_status();
    datagram->setLink_status(link_status);

    datagram->setLoad(controlInfo->getLoad());
    datagram->setNum_packets_for_this_path(controlInfo->getNum_packets_for_this_path());

    // set other fields
    datagram->setDiffServCodePoint(controlInfo->getDiffServCodePoint());
    datagram->setDontFragment (controlInfo->getDontFragment());
    datagram->setFragmentOffset(0);
    datagram->setKind(transportPacket->getKind());

    short ttl;
    if (controlInfo->getTimeToLive() > 0)
        ttl = controlInfo->getTimeToLive();
    else if (datagram->getDestAddress().isLinkLocalMulticast())
        ttl = 1;
    else if (datagram->getDestAddress().isMulticast())
        ttl = defaultMCTimeToLive;
    else
        ttl = defaultTimeToLive;

    datagram->setTimeToLive(ttl);
    datagram->setTransportProtocol(controlInfo->getProtocol());

    // setting IP options is currently supported
    datagram->setOptionCode(7);
    IPRecordRouteOption r;
    short a = r.getNextAddressPtr();
    EV << "Here I am printing the next address pointer" << r.getNextAddressPtr() << "\n";
    if(a < 6)
    {
		r.setRecordAddress(a,rt->getRouterId());
		a++;
		r.setNextAddressPtr(a);
		EV << "After incrementing it is " << a << "\n";
		datagram->setRecordRoute(r);
    }
    return datagram;
}

IPDatagram *IP::createIPDatagram(const char *name)
{
    return new IPDatagram(name);
}

void IP::sendDatagramToOutput(IPDatagram *datagram, InterfaceEntry *ie, IPAddress nextHopAddr)
{
    // hop counter check
    if (datagram->getTimeToLive() <= 0)
    {
        // drop datagram, destruction responsibility in ICMP
        EV << "datagram TTL reached zero, sending ICMP_TIME_EXCEEDED\n";
        icmpAccess.get()->sendErrorMessage(datagram, ICMP_TIME_EXCEEDED, 0);
        return;
    }

    // send out datagram to ARP, with control info attached
    IPRoutingDecision *routingDecision = new IPRoutingDecision();
    routingDecision->setInterfaceId(ie->getInterfaceId());
    routingDecision->setNextHopAddr(nextHopAddr);
    datagram->setControlInfo(routingDecision);
    EV << "Sending datagram along output interface specified" << "\n";
    EV << "The datagram's kind is " << datagram->getKind() << "\n";
    send(datagram, queueOutGate);
    EV << "The arrival gate id for send is " << datagram->getArrivalGateId() << "\n";
    EV << "The arrival gate for send is " << datagram->getArrivalGate() << "\n";
//    cGate *g1 = datagram->getSenderGate();
//    g1->disconnect();
//    if(g1->isConnected())
//    	EV << "The gate is connected " << "\n";
}

int IP::count_pkt_in_queue(int src_addr_in_packet,int dest_addr_in_packet,int srcPort_in_packet,int destPort_in_packet)
{
	int queue_length,count = 0;
	IPAddress dest_addr,src_addr;
	int dest_addr_in_queue,src_addr_in_queue,destPort_in_queue,srcPort_in_queue;
	queue_length = get_Length();
	for(int qq = 0; qq < queue_length ; qq++)
	{
		cPacket *cp = get_ithpacket(qq);
		IPDatagram *itrtr = check_and_cast<IPDatagram *>(cp);
		dest_addr = itrtr->getDestAddress();
		src_addr = itrtr->getSrcAddress();
		dest_addr_in_queue = dest_addr.getInt();
		src_addr_in_queue = src_addr.getInt();
		srcPort_in_queue = itrtr->getSrcPort();
		destPort_in_queue = itrtr->getDestPort();
		if(dest_addr_in_queue == dest_addr_in_packet && src_addr_in_queue == src_addr_in_packet && destPort_in_queue == destPort_in_packet && srcPort_in_queue == srcPort_in_packet)
			count++;
	}
	EV << "The count inside packet count fn is " << count << "\n";
	return count;
}

void IP::fill_delay_load_matrix(int dest_indx, int host_indx, int intrfce_count, int src_host_indx, int dest_host_indx, int packet_count)
{
	int row, num_packets_left_out_to_transmit;
	float total = 0.0;
	int retotal_before = 0, maxi = -1, mini = 99999999;
	int retotal_after = 0, posi_max, posi_min;
	int routes_found_so_far = delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][1][0];
	float temp, temp1;
	int temp2;
	num_packets_left_out_to_transmit = NUM_PACKETS;// - numForwarded_per_host[src_host_indx][host_indx];
	EV << "The total left out packets is " << num_packets_left_out_to_transmit << "\n";
	EV << "The routes found so far are " << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][1][0] << "\n";


	for(row = 0;row < routes_found_so_far;row++)
	{
		EV << "The load for this route is " << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][7] << "\n";
		EV << "The delay for this route is " << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][1] << "\n";
		EV << "The weight for this route is " << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][8] << "\n";
		total += 1 / delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][7];
		EV << "The sum is " << total << "\n";
	}

	for(row = 0;row < routes_found_so_far;row++)
		retotal_before += delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][6];

	retotal_before = -retotal_before;

	EV << "The retotaled value before assignment is" << retotal_before << "\n";

	for(row = 0;row < routes_found_so_far;row++)
	{
		temp = ((1 / delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][7]) * num_packets_left_out_to_transmit) / total;
		temp2 = floor(temp);
		temp1 = temp - temp2;
		EV << "The current value before adjusting " << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][6] << "\n";

		if(temp > 0.5)
		{
			if(temp1 >= 0.5)
				delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][6] += ceil(temp);
			else if(temp1 < 0.5)
				delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][6] += floor(temp);
			if(delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][6] < 0)
				delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][6] = 0;
		}
		else
			delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][6] = 0;

		if(maxi < delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][6])
		{
			maxi = delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][6];
			posi_max = row;
		}

		if(mini > delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][6])
		{
			mini = delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][6];
			posi_min = row;
		}

		EV << "After adjusting " << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][6] << "\n";

		EV << "The number of packets to be sent along route " << row << " is " << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][6] << "\n";
		EV << "The weight of this route is " << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][8] << "\n";
	}

	EV << "The max assignment is " << maxi << " for the path " << posi_max << "\n";
	EV << "The min assignment is " << mini << " for the path " << posi_min << "\n";

	for(row = 0;row < routes_found_so_far;row++)
		retotal_after += delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][6];

	EV << "The retotaled value after assignment is " << retotal_after << "\n";

	if((retotal_before + retotal_after) > NUM_PACKETS)
		delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][posi_max][6] -= (retotal_before + retotal_after) - NUM_PACKETS;
	else if((retotal_before + retotal_after) < NUM_PACKETS)
		delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][posi_min][6] += NUM_PACKETS - (retotal_before + retotal_after);

	EV << "After making necessary changes " << "\n";
	for(row = 0;row < routes_found_so_far;row++)
		EV << "The current assignment for path " << row << " is " << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row][6] << "\n";


//	if(retotal > NUM_PACKETS)
//	{
//		delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row - 1][6] -= (retotal - NUM_PACKETS) ;
//		EV << "Total was higher and after change " << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row - 1][6] << "\n";
//	}
//	else if(retotal < NUM_PACKETS)
//	{
//		delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row - 1][6] += (NUM_PACKETS - retotal) ;
//		EV << "Total was lower and after change " << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][row - 1][6] << "\n";
//	}
}

/*
 * Here the function is called with the destination host id which is obtained from the first datagram which
 * is received. Also the current node's i.e. edge switches address is also passed. Here a packet has to be
 * routed from the current edge switch to the destination edge switch. Hence the destination host id needs
 * to be changed to destination edge switch while sending this probe packet.
 */
IPDatagram *IP::send_probe_packet(IPAddress dest_host, IPAddress src_edge, int src_host_indx, int dest_host_indx)
{
	int i0 = dest_host.getDByte(0);
	int i2 = dest_host.getDByte(2);

	EV << "The first byte of the IP address of destination host is " << i0 << "\n";
	EV << "The third byte of the IP address of destination host is " << i2 << "\n";

	IPAddress dest_edge;
	dest_edge.set(i0,1,i2,0);
	EV << "The modified ip address is " << dest_edge << "\n";

    char msgName[60];
    int protocol = 1;
    InterfaceEntry *destIE;

    EV << "The string representation of ip addresses are " << src_edge.str() << " and " << dest_edge.str() << "\n";
    strcpy(msgName,(char *)(src_edge.str().c_str()));
    strcat(msgName," probe to ");
    strcat(msgName,(char *)(dest_edge.str().c_str()));

    cPacket *payload = new cPacket(msgName,4);
    payload->setByteLength(0);
    EV << "The kind of this packet would be " << payload->getKind() << "\n";
    IPControlInfo *controlInfo = new IPControlInfo();
    controlInfo->setPacketId(0);
    // If the flowid is -1 then it ordinary probe packet which is sent to determine
    // the route. However, if it's value is something else then it would mean that
    // this packet is sent by aggregate switch to edge switch containing the relative
    // edge switch index with respect to destination edge switch.
    controlInfo->setFlowId(-1);

    controlInfo->setLink_status(0);
    // The source port field will contain the index of the host which sends
    // the packet from source to destination.
    controlInfo->setSrcPort(src_host_indx);

    /*
     * The destination port field will contain the index of the host which will
     * receive the packet from source. This is set because it will be used later
     * on in the route_load_packet function for the array delay_load_matrix.
     */
    controlInfo->setDestPort(dest_host_indx);


    controlInfo->setDestAddr(dest_edge);
    controlInfo->setSrcAddr(src_edge);
    controlInfo->setProtocol(protocol);
    controlInfo->setLoad(0);
    controlInfo->setNum_packets_for_this_path(-1);
    payload->setControlInfo(controlInfo);

    IPDatagram *datagram = encapsulate(payload, destIE);
    EV << "After encapsulating the kind value is " << datagram->getKind() << "\n";
    return datagram;
}

void IP::fill_routes_in_delay_load_matrix(int dest_indx, IPAddress aggrg_address, IPAddress core_address, IPAddress aggrg_address_downward, IPAddress edge_address_downward, int num_intrfce, int src_host_indx, int dest_host_indx)
{
	int num_routes_already_found = delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][1][0];
	IPAddress nul(0,0,0,1);
	EV << "Is it coming here in fill route delay" << "\n";
	if(aggrg_address_downward.equals(nul) && edge_address_downward.equals(nul))
	{
		int c;
		for(c = 0; c < num_routes_already_found; c++)
		{
			if(delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][c][2] == (float)aggrg_address.getInt() && delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][c][3] == (float)core_address.getInt())
			{
				delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][c][6]--;
				EV << "The route has already been found just changing this path as used by hash routing" << "\n";
				EV << "The number of hash packet sent along route " << aggrg_address << " is " << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][c][6] << "\n";
				EV << "This packet was received for the dest index " << dest_indx << "\n";
				break;
			}
		}
		if(c == num_routes_already_found)
		{
			delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][num_routes_already_found][2] = aggrg_address.getInt();
			delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][num_routes_already_found][3] = core_address.getInt();
			delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][num_routes_already_found][4] = nul.getInt();
			delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][num_routes_already_found][5] = nul.getInt();
			delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][c][6]--;
			EV << "The number of hash packet sent along route "<< aggrg_address << " is " << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][c][6] << "\n";
			EV << "This packet was received for the dest index " << dest_indx << "\n";
			num_routes_already_found++;
			EV << "This path hasn't been found by probe packets" << "\n";
		}
		EV << "It is received here correctly" << "\n";

	}
	else if(!aggrg_address_downward.equals(nul) && !edge_address_downward.equals(nul))
	{
		int c;
		for(c = 0; c < num_routes_already_found; c++)
		{
			if(delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][c][2] == (float)aggrg_address.getInt() && delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][c][3] == (float)core_address.getInt())
			{
				delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][c][1] = SIMTIME_DBL(simulation.getSimTime()) - delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][c][1];
				delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][c][4] = aggrg_address_downward.getInt();
				delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][c][5] = edge_address_downward.getInt();
				EV << "The route has already been found hence just updating " << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][c][1] << "\n";
				break;
			}
		}
		if(c == num_routes_already_found)// && delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][c][5] == 0)
		{
			delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][num_routes_already_found][1] = SIMTIME_DBL(simulation.getSimTime()) - delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][num_routes_already_found][1];
			delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][num_routes_already_found][2] = aggrg_address.getInt();
			delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][num_routes_already_found][3] = core_address.getInt();
			delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][num_routes_already_found][4] = aggrg_address_downward.getInt();
			delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][num_routes_already_found][5] = edge_address_downward.getInt();
			EV << "The route hasn't been found hence adding new row " << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][num_routes_already_found][1] << "\n";
			num_routes_already_found++;
		}
	}
	delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][1][0] = num_routes_already_found;
	EV << "The updated next hop to aggregate switch" << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][num_routes_already_found - 1][2] << "\n";
	EV << "The updated next hop to core switch" << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][num_routes_already_found - 1][3] << "\n";
	EV << "The updated next hop to aggregate switch downward" << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][num_routes_already_found - 1][4] << "\n";
	EV << "The updated next hop to core switch downward" << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][num_routes_already_found - 1][5] << "\n";
	EV << "Routes so far found are " << delay_load_matrix[dest_indx][src_host_indx][dest_host_indx][1][0] << "\n";
}

void IP::send_load_estimation_packet(IPAddress srcadr, IPAddress dstadr, int src_host_indx, int dest_host_indx)
{
	IPAddress dst_edge, src_edge;
	int i0 = srcadr.getDByte(0);
	int i2 = srcadr.getDByte(2);
	src_edge.set(i0,1,i2,0);
    IPAddress nextHopAddr;

	i0 = dstadr.getDByte(0);
	i2 = dstadr.getDByte(2);
	dst_edge.set(i0,1,i2,0);
	EV << "The destination edge switch is " << dst_edge <<"\n";
	EV << "The source edge switch is " << src_edge <<"\n";

    char msgName[60];
    int protocol = 46;
    InterfaceEntry *destIE;
    const IPRoute *probe_route = NULL;

    EV << "The string representation of ip addresses are " << src_edge.str() << " and " << dst_edge.str() << "\n";

    strcpy(msgName,"Load from ");
    strcat(msgName,(char *)(src_edge.str().c_str()));
    strcat(msgName," to ");
    strcat(msgName,(char *)(dst_edge.str().c_str()));

    cPacket *payload = new cPacket(msgName,3);
    payload->setByteLength(0);

    IPControlInfo *controlInfo = new IPControlInfo();
    // Here the packet id would contain the number of interfaces connected to an non-controller
    // switch. This would be useful when the packet is received by the controller and the controller
    // would decide to which all
    controlInfo->setPacketId(ift->getNumInterfaces() - 2);

    // The src port field will contain the source host index which will be useful later on while
    // filling the load in the matrix.
    controlInfo->setSrcPort(src_host_indx);

    // Similarly for the destination as well.
    controlInfo->setDestPort(dest_host_indx);

    controlInfo->setDestAddr(dst_edge);
    controlInfo->setSrcAddr(src_edge);
    controlInfo->setProtocol(protocol);

    controlInfo->setLoad(-1);
    controlInfo->setNum_packets_for_this_path(-1);
    payload->setControlInfo(controlInfo);

    IPDatagram *datagram = encapsulate(payload, destIE);

    // The last route in the rt table is the one connected to the controller.
	probe_route = rt->getRoute(rt->getNumRoutes() - 1);
	destIE = probe_route->getInterface();
	nextHopAddr = probe_route->getGateway();

	fragmentAndSend(datagram, destIE, nextHopAddr);
}

void IP::route_load_packet(IPDatagram *datagram, InterfaceEntry *destIE, bool fromHL)
{
	/*
	 * Check if the router which processes this packet is a controller. If so then it
	 * has received a packet from the source edge switch to identify the load of all
	 * parallel paths to destination. If this switch is not a controller & if it is not
	 * a source then all it needs to do is return its load which would be queue length.
	 * If it is a source then it has to record the received value in the delay load matrix.
	 */
	// The packet id will contain the number of interfaces of the other switches.
	IPAddress current_router_id = rt->getRouterId();
	const IPRoute *load_route = NULL;
	IPAddress nextHopAddr;
	IPDatagram *d;
	IPAddress src_address = datagram->getSrcAddress();
	IPAddress dst_address = datagram->getDestAddress();

	// Only the controller will have the layer id, switch id and host id as 0. No other
	// node will have it this way.
	if(current_router_id.getDByte(1) == 0 && current_router_id.getDByte(2) == 0 && current_router_id.getDByte(3) == 0)
	{
		// Here -1 is used below while sending new load packet from the source edge switch.
		// Later on when the controller receives reply from the other nodes the load
		// value would have changed and hence it can be used to distinguish if the packet
		// came from the source edge switch or from the other nodes.
		if(datagram->getLoad() == -1)
		{
			int num_intrfce = ift->getNumInterfaces() - 1;
			EV << "The number of interfaces are " << num_intrfce << "\n";

			int source_switch_id = src_address.getDByte(2);
			EV << "The source switch id is " << source_switch_id << "\n";
			int dest_switch_id = dst_address.getDByte(2);
			EV << "The dest switch id is " << dest_switch_id << "\n";
			int c;

			// This variable represent the number of interfaces of a non-controller.
			int num_intrfce_other_node = datagram->getPacketId();

			EV << "The number of interface of other nodes are " << num_intrfce_other_node << "\n";

			// This variable represent the number of edge or aggregate switches.
			int num_aggrg_present = (num_intrfce_other_node * num_intrfce_other_node) / 2;
			EV << "The number of aggregates present " << num_aggrg_present << "\n";

			int pod_id_source = src_address.getDByte(0);
			EV << "The pod id of the source is " << pod_id_source << "\n";

			int pod_id_dest = dst_address.getDByte(0);
			EV << "The pod id of the dest is " << pod_id_dest << "\n";

			int pod_id_core = (num_intrfce_other_node + 1) * 10;
			EV << "The pod id of the core is " << pod_id_core << "\n";

			int layer_id = 2;
			IPAddress temp;

			// Here the value below is divided by 2 because the number of edge or aggregate
			// switches present are half the number of their interfaces.
			num_intrfce_other_node /= 2;

			while(source_switch_id % num_intrfce_other_node != 1)
				source_switch_id--;
			EV << "The source switch id after decrementing " << source_switch_id << "\n";

			while(dest_switch_id % num_intrfce_other_node != 1)
				dest_switch_id--;
			EV << "The destination switch id after decrementing " << dest_switch_id << "\n";

			// This number represent the starting id of the aggregate switch in the pod to which
			// the source edge switch belongs.
			int source_start_aggregate_switch = source_switch_id - num_aggrg_present;
			EV << "The source start aggregate switch is " << source_start_aggregate_switch << "\n";

			// This number represent the starting id of the aggregate switch in the pod to which
			// the destination edge switch belongs.
			int dest_start_aggregate_switch = dest_switch_id - num_aggrg_present;
			EV << "The dest start aggregate switch is " << dest_start_aggregate_switch << "\n";

			// Here c is initialised to one, because 0th interface is loopback and packet has to
			// be sent on all the interface except loop-back.

			EV << "The changed number of interfaces of the other nodes is " << num_intrfce_other_node << "\n";
			/*
			 * This do-while loop below would send the packets to all the aggregate switches
			 * in the pod to which the edge switches belong.
			 */
			do
			{
				temp.set(pod_id_source,layer_id,source_start_aggregate_switch,0);
				EV << "The temporary IP address generated from the id's are " << temp << "\n";
				for(c = 0; c < rt->getNumRoutes(); c++)
				{
					load_route = rt->getRoute(c);
					if(temp.equals(load_route->getHost()))
						break;
				}
				load_route = rt->getRoute(c);
				destIE = load_route->getInterface();
				EV << "The interface where I want to break the link is " << destIE->getName() << "\n";
				EV << "The datagram arrival gate is " << datagram->getArrivalGate() << "\n";
				EV << "The datagram sender gate is " << datagram->getSenderGate() << "\n";
//				cGate *g1 = datagram->getSenderGate();
//				g1->disconnect();
//				if(!g1->isConnected())
//					EV << "The gate has been disconnected successfully " << "\n";
				nextHopAddr = load_route->getGateway();
				d = datagram->dup();
				//destIE->setDown(true);
				EV << "Testing for link downness " << destIE->isDown() << "\n";
				fragmentAndSend(d, destIE, nextHopAddr);
				source_start_aggregate_switch++;
			}while(source_start_aggregate_switch % num_intrfce_other_node != 1);

			if(src_address.getDByte(0) != dst_address.getDByte(0))
			{
				do
				{
					temp.set(pod_id_dest,layer_id,dest_start_aggregate_switch,0);
					EV << "The temporary IP address generated from the id's are " << temp << "\n";
					for(c = 0; c < rt->getNumRoutes(); c++)
					{
						load_route = rt->getRoute(c);
						if(temp.equals(load_route->getHost()))
							break;
					}
					load_route = rt->getRoute(c);
					destIE = load_route->getInterface();
					EV << "The interface is " << destIE->getName() << "\n";
					nextHopAddr = load_route->getGateway();
					d = datagram->dup();
					fragmentAndSend(d, destIE, nextHopAddr);
					dest_start_aggregate_switch++;
				}while(dest_start_aggregate_switch % num_intrfce_other_node != 1);

				// Now the packets have to be sent to core switches.
				for(c = 0; c < rt->getNumRoutes(); c++)
				{
					load_route = rt->getRoute(c);
					temp = load_route->getHost();
					if(temp.getDByte(0) == pod_id_core)
					{
						destIE = load_route->getInterface();
						EV << "The interface is " << destIE->getName() << "\n";
						nextHopAddr = load_route->getGateway();
						d = datagram->dup();
						fragmentAndSend(d, destIE, nextHopAddr);
					}
				}
			}
			delete datagram; //Delete the original datagram as copies of it are sent
		}
		else if(datagram->getLoad() != -1)
		{
			load_route = rt->findBestMatchingRoute(src_address);
			destIE = load_route->getInterface();
			EV << "The interface is " << destIE->getName() << "\n";
			nextHopAddr = load_route->getGateway();
			fragmentAndSend(datagram, destIE, nextHopAddr);
		}
	}
	else if(!src_address.equals(rt->getRouterId()))
	{
		// This is for the other switches present in the topology. They just return
		// the packet with their load.

		// Here the output port to core switch field is used to store the original
		// destination address of the edge switch for which the load is determined.
		datagram->setOutput_port_to_core_switch(dst_address.getInt());
		// The destination address is set to the current router id.
		datagram->setDestAddress(rt->getRouterId());
		EV << "After changing the src address is " << datagram->getSrcAddress() << "\n";
		EV << "After changing the dest address is " << datagram->getDestAddress() << "\n";
		EV << "The old dest address is " << datagram->getOutput_port_to_core_switch() << "\n";
		int queue_len = get_Length() + future_packets_count;
		EV << "The current queue length of this node " << rt->getRouterId() << " is " << queue_len<< "\n";
		if(queue_len != 0)
			datagram->setLoad((float)queue_len);
		else
			// While calculating the ratio of packets to be sent inverse of the load is
			// taken into consideration. Hence if the queue length is 0 the value is set to 0.1(small value).
			datagram->setLoad(1 + queue_len);

		load_route = rt->getRoute(rt->getNumRoutes() - 1);
		numForwarded_load_packet++;
		destIE = load_route->getInterface();
		nextHopAddr = load_route->getGateway();
		fragmentAndSend(datagram, destIE, nextHopAddr);
	}
	else if(src_address.equals(rt->getRouterId()))
	{
		int num_intrfce = ift->getNumInterfaces() - 2;
		uint32 old_dest_addr_int = datagram->getOutput_port_to_core_switch();
		uint32 packets_dest_addr_int = dst_address.getInt();
		float load = datagram->getLoad();

		// Temp variable contains the destination address to which the actual packet was destined.
		IPAddress temp(old_dest_addr_int);

		// The source port contains the source host which is sending the packet.
		// The destination port contains the destination host to which the packet
		// is destined.
		int src_host_indx = datagram->getSrcPort();
		int dest_host_indx = datagram->getDestPort();

		int dest_switch_id = temp.getDByte(2);
		int src_switch_id =  src_address.getDByte(2);

		int index_difference = dest_switch_id - src_switch_id;
		int relative_destination_edge_index = (index_difference > 0) ? index_difference: (index_difference + ((num_intrfce * num_intrfce) / 2));
		int num_routes_for_this_packet;

		if(temp.getDByte(0) != src_address.getDByte(0))
			num_routes_for_this_packet = num_intrfce;
		else
			num_routes_for_this_packet = num_intrfce / 2;

		for(int c = 0; c < num_routes_for_this_packet; c++)
		{
			for(int d = 2; d <= 4; d++)
			{
				if(delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][c][d] == packets_dest_addr_int && delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][c][7] <= load)
				{
					EV << "The destination address in the matrix is " << delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][c][d] << "\n";
					EV << "The packet is obtained from the switch " << dst_address << "\n";
					EV << " The position in the matrix is " << c << " " << d << "\n";
					delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][c][7] = load;
					break;
				}
			}
		}
		numReceived_load_packet++;
		delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][2][0]++;
		EV << "The number of packets obtained are " << delay_load_matrix[relative_destination_edge_index][src_host_indx][dest_host_indx][2][0] << "\n";
		delete datagram;
	}
}

void IP::finish()
{
    recordScalar("Dynamic routing packets forwarded", numForwarded_dynamic_routing_packet);
    recordScalar("Hash routing packets forwarded", numForwarded_hash_routing_packet);
    recordScalar("Direct routing packets forwarded", numForwarded_direct_routing_packet);
    recordScalar("Load packets forwarded", numForwarded_load_packet);
    recordScalar("Load packets sent", numSent_load_packet);
    recordScalar("Load packets received", numReceived_load_packet);
    recordScalar("Probe packets forwarded", numForwarded_probe_packet);
    recordScalar("Probe packets sent", numSent_probe_packet);
    recordScalar("Probe packets received", numReceived_probe_packet);
}

void IP::send_future_packet_count(IPDatagram *base_datagram, IPAddress srcaddress, IPAddress dstaddress, int relative_destination_edge_index, int src_indx, int dest_indx, int num_routes)
{
    char msgName[40];
    int protocol = 1;
    int upto_column_in_matrix;
    IPAddress edge_start = rt->getRouterId();
    IPAddress nul(0,0,0,0);
    int i = 0;

    strcpy(msgName,"Packet count from ");
    strcat(msgName,(char *)(edge_start.str().c_str()));

    cPacket *payload = new cPacket(msgName,5);
    payload->setByteLength(0);

    IPControlInfo *controlInfo = new IPControlInfo();
    IPRecordRouteOption r;
    IPDatagram *duplcte;
    InterfaceEntry *destIE;

    // Here packetId will contain the num_routes which will be used later on.
    controlInfo->setPacketId(num_routes);
    controlInfo->setSrcPort(0);
    controlInfo->setDestPort(0);
    controlInfo->setDestAddr(nul);
    controlInfo->setSrcAddr(edge_start);
    controlInfo->setProtocol(protocol);
    controlInfo->setLoad(0);
    controlInfo->setNum_packets_for_this_path(0);
    payload->setControlInfo(controlInfo);

    IPDatagram *datagram = encapsulate(payload, destIE);

    if(srcaddress.getDByte(0) == dstaddress.getDByte(0))
    	upto_column_in_matrix = 2;
    else
    	upto_column_in_matrix = 4;

    for(int c = 0; c < num_routes; c++)
    {
    	r.setRecordAddress(i++,rt->getRouterId());
		r.setNextAddressPtr(i);
		datagram->setRecordRoute(r);
    	for(int d = 2; d <= upto_column_in_matrix; d++)
    	{
    		r.setRecordAddress(i++, delay_load_matrix[relative_destination_edge_index][src_indx][dest_indx][c][d]);
    		r.setNextAddressPtr(i);
    		datagram->setRecordRoute(r);
    	}
    	datagram->setNum_packets_for_this_path(delay_load_matrix[relative_destination_edge_index][src_indx][dest_indx][c][6]);
    	duplcte = datagram->dup();
    	duplcte->setArrival(base_datagram->getArrivalModule(),base_datagram->getArrivalGateId());
    	pqueue.insert(duplcte);
    	i = 0;
    }
    delete datagram;
}

void IP::route_link_failure_packet(IPDatagram *datagram, InterfaceEntry *destIE, bool fromHL)
{
	/*
	 * The source switch which detected the link failure sends the link failure packet to the
	 * controller, which in turn send the link failure packet to all the edge switches. The edge
	 * switches upon receiving the packet updates in its matrix about the failure and adjusts the
	 * packet accordingly along the rest of the paths.
	 */
	// The packet id will contain the number of interfaces of the other switches.
	IPAddress current_router_id = rt->getRouterId();
	const IPRoute *load_route = NULL;
	IPAddress nextHopAddr, temp;
	IPDatagram *d;
	int num_interface_for_this_node = ift->getNumInterfaces() - 2;

	// Only the controller will have the layer id, switch id and host id as 0. No other
	// node will have it this way.
	if(current_router_id.getDByte(1) == 0 && current_router_id.getDByte(2) == 0 && current_router_id.getDByte(3) == 0)
	{
		// Now the packets have to be sent to all the edge switches.
		for(int c = 0; c < rt->getNumRoutes(); c++)
		{
			load_route = rt->getRoute(c);
			temp = load_route->getHost();
			if(temp.getDByte(1) == 1)
			{
				destIE = load_route->getInterface();
				EV << "The interface is " << destIE->getName() << "\n";
				nextHopAddr = load_route->getGateway();
				d = datagram->dup();
				fragmentAndSend(d, destIE, nextHopAddr);
			}
		}
		delete datagram;
	}
	else
	{
		uint32 src_link_failure_address = datagram->getSrcAddress().getInt();
		uint32 dest_link_failure_address = datagram->getDestAddress().getInt();
		int num_routes_found;

		// Check if the source or the destination router is the current edge switch. This check is needed because if the
		// link failure is between an edge and aggregate switch then, this failure information is received by all the edge
		// switches. The edge switch whose link failed will not have its corresponding address in any of the paths stored
		// in the matrix. However other switches will have the failed link's edge switch in the matrix. Hence this check
		// is needed for those switches whose corresponding link failed.
		if(current_router_id.getInt() == src_link_failure_address)
		{
			EV << "The failed source link is " << current_router_id << "\n";
			EV << "The failed dest link is " << dest_link_failure_address << "\n";
			// Since the source address of failed link and the current router id matches this check is for the edge
			// switches. Hence the search is for the aggregate switch in delay load matrix matching with the destination
			// address of failed link.
			for(int c = 0; c < num_interface_for_this_node * 2; c++)
			{
				for(int d = 0; d < num_interface_for_this_node / 2; d++)
				{
					for(int e = 0; e < num_interface_for_this_node / 2; e++)
					{
					    num_routes_found = delay_load_matrix[c][d][e][1][0];
						for(int row = 0; row < num_routes_found; row++)
						{
							for(int col = 2; col <= 5 ; col++)
							{
								if(dest_link_failure_address == delay_load_matrix[c][d][e][row][col])
								{
									// Match has been found and the packet count has to be adjusted with the
									// rest of the paths.
									adjust_packet_count(c,d,e,row,num_routes_found);
								}
							}
						}
					}
				}
			}
		}
////		else if (current_router_id.getInt() == dest_link_failure_address)
////		{
////
////		}
		else
		{

			EV << "It has come into this printing" << "\n";
//			// For the rest of the cases of link failure. This one suits well for core to aggregate link failure.
//
//			EV << "The failed src and dest link respectively are " << src_link_failure_address << " " << dest_link_failure_address << "\n";
//			for(int c = 0; c < num_interface_for_this_node * 2; c++)
//			{
//				for(int d = 0; d < num_interface_for_this_node / 2; d++)
//				{
//					for(int e = 0; e < num_interface_for_this_node / 2; e++)
//					{
//					    num_routes_found = delay_load_matrix[c][d][e][1][0];
//						for(int row = 0; row < num_routes_found; row++)
//						{
//							for(int col = 2; col < 5 ; col++)
//							{
//								if((src_link_failure_address == delay_load_matrix[c][d][e][row][col] && dest_link_failure_address == delay_load_matrix[c][d][e][row][col + 1]) | (dest_link_failure_address == delay_load_matrix[c][d][e][row][col] && src_link_failure_address == delay_load_matrix[c][d][e][row][col + 1]))
//								{
//									// Match has been found and the packet count has to be adjusted with the
//									// rest of the paths.
//									adjust_packet_count(c,d,e,row,num_routes_found);
//								}
//							}
//						}
//					}
//				}
//			}
		}
	}
}

void IP::adjust_packet_count(int c, int d, int e, int row, int num_routes_found)
{
// Here the number of left out packets are obtained in that particular row and is realloted to the other routes.
// Then that corresponding paths packet count is set to -1. It is set to -1 here so that when we check to balance
// the packets among the other nodes they would have non-zero packet count.

	int packet_left_along_this_path = delay_load_matrix[c][d][e][row][6];
	float total = 0.0;
	float temp, temp1;
	int temp2;

	delay_load_matrix[c][d][e][row][6] = -1;

	for(int roh = 0; roh < num_routes_found; roh++)
	{
		if(delay_load_matrix[c][d][e][roh][6] != -1)
			total += 1 / delay_load_matrix[c][d][e][roh][7];
		EV << "The packet before adjusting along this path" << roh << " is " << delay_load_matrix[c][d][e][roh][6] << "\n";
	}

	for(int roh = 0; roh < num_routes_found; roh++)
	{
		if(delay_load_matrix[c][d][e][roh][6] != -1)
		{
			temp = ((1 / delay_load_matrix[c][d][e][roh][7]) * packet_left_along_this_path) / total;
			temp2 = floor(temp);
			temp1 = temp - temp2;

			if(temp > 0.5)
			{
				if(temp1 >= 0.5)
					delay_load_matrix[c][d][e][roh][6] += ceil(temp);
				else if(temp1 < 0.5)
					delay_load_matrix[c][d][e][roh][6] += floor(temp);
				if(delay_load_matrix[c][d][e][roh][6] < 0)
					delay_load_matrix[c][d][e][roh][6] = 0;
			}
		}
		EV << "The packet after adjusting along this path" << roh << " is " << delay_load_matrix[c][d][e][roh][6] << "\n";
	}
}
