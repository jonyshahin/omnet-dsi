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


#include "QueueBase.h"
#include "IPDatagram.h"


void QueueBase::initialize()
{
	EV << "Initializing Queuebase" << "\n";
    AbstractQueue::initialize();
    delay = par("procDelay");
}

void QueueBase::arrival(cPacket *msg)
{
	EV << "When is it arriving here in Queuebase?" << "\n";
	cPacket *p1 = msg->getEncapsulatedMsg();
	if(p1) // To insert into pqueue always encapsulate the packet and then send.
	{
		EV << "The packet contains encapsulated message" << "\n";
		IPDatagram *ipd = check_and_cast<IPDatagram *>(msg);
		EV << "The encapsulated message contains the source address as " << ipd->getSrcAddress() << "\n";
		if(ipd->getTransportProtocol() == 1 || ipd->getTransportProtocol() == 46 || ipd->getTransportProtocol() == 2)
		{
			EV << "Inserting this message into pqueue " << "\n";
			pqueue.insert(msg);
		}
		else
			queue.insert(msg);
	}
	else
	{
		EV << "The packet doesn't contain any encapsulated message" << "\n";
		queue.insert(msg);
	}

/*	if(ipd->getTransportProtocol() == 1)
		EV << "Pqueue" << "\n", pqueue.insert(msg);*/
	//else (msg->getArrivalGate()->isName("transportIn") || msg->getArrivalGate()->isName("udpIn"))
		//queue.insert(msg);
}

int QueueBase::get_Length()
{
	return queue.length();
}

cPacket *QueueBase::get_ithpacket(int i)
{
	return queue.get(i);
}

cPacket *QueueBase::arrivalWhenIdle(cPacket *msg)
{
	EV << "Arrival when idle is called in Queuebase" << "\n";
	cPacket *p1 = msg->getEncapsulatedMsg();
	if(p1)
	{
		EV << "The packet contains encapsulated message" << "\n";
		IPDatagram *ipd = check_and_cast<IPDatagram *>(msg);
		EV << "The encapsulated message contains the source address as " << ipd->getSrcAddress() << "\n";
		if(ipd->getTransportProtocol() == 1 || ipd->getTransportProtocol() == 46 || ipd->getTransportProtocol() == 2)
		{
			EV << "Inserting this message into pqueue " << "\n";
			pqueue.insert(msg);
			return NULL;
		}
		else
			return msg;
	}
	else
		return msg;
}

simtime_t QueueBase::startService(cPacket *msg)
{
	EV << "It has certain delay in Queuebase" << "\n";
    return delay;
}

