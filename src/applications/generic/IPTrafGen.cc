//
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
// Copyright (C) 2004-2005 Andras Varga
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
#include "IPTrafGen.h"
#include "IPControlInfo.h"
#include "IPv6ControlInfo.h"
#include "IPAddressResolver.h"

Define_Module(IPTrafSink);

void IPTrafSink::initialize()
{
	order = 1;
	last_packet_id = 0;
    numReceived = 0;
    WATCH(numReceived);
}

void IPTrafSink::handleMessage(cMessage *msg)
{
    processPacket(check_and_cast<cPacket *>(msg));

    if (ev.isGUI())
    {
        char buf[32];
        sprintf(buf, "rcvd: %d pks", numReceived);
        getDisplayString().setTagArg("t",0,buf);
    }

}

void IPTrafSink::printPacket(cPacket *msg)
{
    IPvXAddress src, dest;
    int protocol = -1;
    int flowId, packetId;
    if (dynamic_cast<IPControlInfo *>(msg->getControlInfo())!=NULL)
    {
        IPControlInfo *ctrl = (IPControlInfo *)msg->getControlInfo();
        src = ctrl->getSrcAddr();
        dest = ctrl->getDestAddr();
        flowId = ctrl->getFlowId();
        packetId = ctrl->getPacketId();
        if(packetId > 0)
        {
        	if(packetId == last_packet_id + 1)
        	{
        		last_packet_id = packetId;
        		order++;
        	}
        	else
        		order--;
        }
        protocol = ctrl->getProtocol();
    }
    else if (dynamic_cast<IPv6ControlInfo *>(msg->getControlInfo())!=NULL)
    {
        IPv6ControlInfo *ctrl = (IPv6ControlInfo *)msg->getControlInfo();
        src = ctrl->getSrcAddr();
        dest = ctrl->getDestAddr();
        protocol = ctrl->getProtocol();
    }

    ev  << msg << endl;
    ev  << "Payload length: " << msg->getByteLength() << " bytes" << endl;
    if (protocol!=-1)
        ev  << "src: " << src << " dest: " << dest << " flow: " << flowId << " packet: " << packetId << " protocol:" << protocol << "\n";
}

void IPTrafSink::processPacket(cPacket *msg)
{
    EV << "Received packet: ";
    printPacket(msg);
    delete msg;

    numReceived++;
}


//===============================================


Define_Module(IPTrafGen);

//int IPTrafGen::counter;

int IPTrafGen::map_bijective[16];

void IPTrafGen::initialize(int stage)
{
    // because of IPAddressResolver, we need to wait until interfaces are registered,
    // address auto-assignment takes place etc.
    if (stage!=3)
        return;
    rt = RoutingTableAccess().get();
    EV << "Coming into IPTraffic generator" << "\n";
    EV << "The router Id of this generator is " << rt->getRouterId() << "\n";
    IPTrafSink::initialize();

    cOutVector endToEndDelay;
    endToEndDelay.setName("end-to-end");

    protocol = par("protocol");
    msgByteLength = par("packetLength");
    numPackets = par("numPackets");
    numFlows = par("numFlows");

    simtime_t startTime = par("startTime");

    const char *destAddrs = par("destAddresses");
    cStringTokenizer tokenizer(destAddrs);
    const char *token;
    while ((token = tokenizer.nextToken())!=NULL)
        destAddresses.push_back(IPAddressResolver().resolve(token));

    const char *destPort = par("destPort");
    cStringTokenizer tokenizer1(destPort);
    int token1;
    while ((token1 = int(tokenizer1.nextToken()))!=NULL)
        destPorts.push_back(token1);

    const char *srcPort = par("srcPort");
    cStringTokenizer tokenizer2(srcPort);
    while ((token1 = int(tokenizer2.nextToken()))!=NULL)
        srcPorts.push_back(token1);

    counter = 0;

    numSent = 0;
    numFlowcount = 0;

    WATCH(numSent);

    if (destAddresses.empty())
        return;

    if (numPackets > 0) {
        cMessage *timer = new cMessage("sendTimer");
        scheduleAt(startTime, timer);
    }

    for(int c = 0;c < 16;c++)
    {
    	map_bijective[c] = -1;
    }
}

void IPTrafGen::generate_bijective_mapping(int indx)
{
	int rnd_num = intrand(destAddresses.size());
	int c = 0;
	while(c < 16) //change this 16. Here it is 16 coz there are 16 hosts.
	{
		if(map_bijective[c] == rnd_num || (rnd_num + 1) == indx)
		{
			rnd_num = intrand(destAddresses.size());
			c = -1;
		}
		c++;
		if(c == 16)
		{
			map_bijective[indx] = rnd_num;
			break;
		}
	}
	EV << "The bijective mapping value for the router " << rt->getRouterId() << " is " << map_bijective[indx] << "\n";
}

IPvXAddress IPTrafGen::chooseDestAddr(int indx)
{

	/*
	 * For stride use the below return value
	 */
	if(numFlowcount < numFlows)
		return destAddresses[numFlowcount];

	/*
	 * For bijective mapping use the below return value.
	 */
		//return destAddresses[map_bijective[indx]];

	/*
	 * For random generations use the below return value
	 */
    //int k = intrand(destAddresses.size());
	// return k;
}

int IPTrafGen::chooseDestPortNum(int indx)
{
	/*
	 * For stride use the below return value
	 */
	if(numFlowcount < numFlows)
		return destPorts[numFlowcount];
	/*
	 * For bijective mapping use the below return value.
	 */
		//return destPorts[map_bijective[indx]];

	/*
	 * For random generations use the below return value
	 */
	//int k = intrand(destPorts.size());
	// return k;
}

int IPTrafGen::chooseSrcPortNum(int indx)
{
	/*
	 * For stride use the below return value
	 */
	if(numFlowcount < numFlows)
		return srcPorts[numFlowcount];
	/*
	 * For bijective mapping use the below return value.
	 */
		//return srcPorts[map_bijective[indx]];

	/*
	 * For random number generations use the below return value
	 */
	//int k = intrand(srcPorts.size());
	// return k;
}

void IPTrafGen::sendPacket()
{
    char msgName[32];
    sprintf(msgName,"packet-%d", counter++);
	IPAddress this_node = rt->getRouterId();
    cPacket *payload = new cPacket(msgName);
    payload->setByteLength(msgByteLength);

    /*
     * Uncomment the below lines for bijective mapping generation
     */
//    if(numSent == 0)
//    {
//    	generate_bijective_mapping((this_node.getDByte(2) - 13) * 2 + this_node.getDByte(3)); //Change this 13. Here it is 13 coz there are 4 core and 8 aggregate switches and the numbering starts from 1.
//    	//Change this bijective mapping appropriately....... Here it is multiplied by 2 coz there are 2 hosts in 4 port network.
//    }

    IPvXAddress destAddr = chooseDestAddr((this_node.getDByte(2) - 13) * 2 + this_node.getDByte(3));
    int destPort = chooseDestPortNum((this_node.getDByte(2) - 13) * 2 + this_node.getDByte(3));
    int srcPort = chooseSrcPortNum((this_node.getDByte(2) - 13) * 2 + this_node.getDByte(3));
    EV << "The dest address chosen is " << destAddr << "\n";
    EV << "The dest port chosen is " << destPort << "\n";
    EV << "The src port chosen is " << srcPort << "\n";
    if (!destAddr.isIPv6())
    {
        // send to IPv4
        IPControlInfo *controlInfo = new IPControlInfo();
        controlInfo->setPacketId(numSent);
        controlInfo->setFlowId(numFlowcount);
        controlInfo->setDestAddr(destAddr.get4());
        controlInfo->setProtocol(protocol);
        controlInfo->setNum_packets_for_this_path(-1);
        controlInfo->setDestPort(destPort);
        controlInfo->setSrcPort(srcPort);
        payload->setControlInfo(controlInfo);

        EV << "Sending packet: ";
        printPacket(payload);

        send(payload, "ipOut");
    }
    else
    {
        // send to IPv6
        IPv6ControlInfo *controlInfo = new IPv6ControlInfo();
        controlInfo->setDestAddr(destAddr.get6());
        controlInfo->setProtocol(protocol);
        payload->setControlInfo(controlInfo);

        EV << "Sending packet: ";
        printPacket(payload);

        send(payload, "ipv6Out");
    }
    numSent++;
    if(numSent == numPackets)
    {
    	numSent = 0;
		numFlowcount++;
    }
}

void IPTrafGen::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        // send, then reschedule next sending
        sendPacket();

        if (!numPackets || numFlowcount < numFlows)
            scheduleAt(simTime()+(double)par("packetInterval"), msg);
        else
            delete msg;
    }
    else
    {
        // process incoming packet
        processPacket(PK(msg));
        simtime_t vec = simTime() - msg->getCreationTime();
        endToEndDelay.record(vec);
    }

    if (ev.isGUI())
    {
        char buf[40];
        sprintf(buf, "rcvd: %d pks\nsent: %d pks", numReceived, numSent);
        getDisplayString().setTagArg("t",0,buf);
    }
}

void IPTrafGen::finish()
{
    //recordScalar("packets sent", numSent);
    //recordScalar("packets received", numReceived);
	recordScalar("Simulation end time",simTime());
	recordScalar("Arrival order",order);
}
