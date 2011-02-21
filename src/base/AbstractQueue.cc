//
// Copyright (C) 2004, 2008 Andras Varga
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
#include "AbstractQueue.h"


AbstractQueue::AbstractQueue()
{
    msgServiced = NULL;
    endServiceMsg = NULL;
    probe_endServiceMsg = NULL;
}

AbstractQueue::~AbstractQueue()
{
    delete msgServiced;
    cancelAndDelete(endServiceMsg);
    cancelAndDelete(probe_endServiceMsg);
}

void AbstractQueue::initialize()
{
	EV << "Initializing AbstractQueue" << "\n";
    msgServiced = NULL;
    probe_endServiceMsg = new cMessage("probe-message");
    endServiceMsg = new cMessage("end-service");
    queue.setName("queue");
    pqueue.setName("probe_queue");
}

void AbstractQueue::handleMessage(cMessage *msg)
{
	EV << "How about handling message here in AbstractQueue?" << "\n";

    if (msg==endServiceMsg)
    {
        doEndService();
    }
    else if(msg == probe_endServiceMsg)
    {
    	doProbeEndService();
    }
    else if (!msgServiced)
    {
        cPacket *msg2 = arrivalWhenIdle( PK(msg) );
        if (msg2)
        {
            msgServiced = msg2;
            doStartService();
        }
        if(!probe_endServiceMsg->isScheduled())
        {
        	serviceTime = startService( PK(msg) );
        	EV << "Looking for service time value " << serviceTime << "\n";
        	scheduleAt(simTime() + (serviceTime / 10),probe_endServiceMsg);
        }
    }
    else
    {
        arrival( PK(msg) );
        if(!probe_endServiceMsg->isScheduled())
        {
        	serviceTime = startService( PK(msg) );
        	EV << "Looking for service time value " << serviceTime << "\n";
        	scheduleAt(simTime() + (serviceTime / 10),probe_endServiceMsg);
        }
    }
}

void AbstractQueue::doStartService()
{
	EV << "Starting service in AbstractQueue?" << "\n";
    serviceTime = startService( msgServiced );
/*	cPacket *p1 = msgServiced->getEncapsulatedMsg();
	if(p1)
	{
		EV << "The packet contains encapsulated message" << "\n";
		IPDatagram *ipd = check_and_cast<IPDatagram *>(msgServiced);
		EV << "The encapsulated message contains the source address as " << ipd->getSrcAddress() << "\n";
		if(ipd->getTransportProtocol() == 1 || ipd->getTransportProtocol() == 46)
		{
			EV << "Is it setting the value after knowing empty queue" << "\n";
	        if(!probe_endServiceMsg->isScheduled())
	        	scheduleAt(simTime() + (serviceTime / 10),probe_endServiceMsg);
		}
		else
		{*/
	if (serviceTime != 0)
		scheduleAt( simTime()+serviceTime, endServiceMsg );
	else
		doEndService();
		/*}
	}
	else
	{
	    if (serviceTime != 0)
	        scheduleAt( simTime()+serviceTime, endServiceMsg );
	    else
	        doEndService();
	}*/
}

void AbstractQueue::doEndService()
{
	EV << "Finishing message service in AbstractQueue" << "\n";
    endService( msgServiced );
    if (queue.empty())
    {
        msgServiced = NULL;
    }
    else
    {
    	EV << "When is it popping here in end service " << "\n";
        msgServiced = queue.pop();
        doStartService();
    }
}

void AbstractQueue::doProbeEndService()
{
	EV << "Wait complete for the probe message" << "\n";
	/*if(pqueue.empty())
	{
		endService(msgServiced);
	}
	else*/
	if(!pqueue.empty())
	{
		endService(pqueue.pop());
		if(!probe_endServiceMsg->isScheduled())
		{
			EV << "Looking for service time value " << serviceTime << "\n";
			scheduleAt(simTime() + (serviceTime / 10),probe_endServiceMsg);
		}
	}
/*	if(!probe_endServiceMsg->isScheduled())
	{
				EV << "Looking for service time value " << serviceTime << "\n";
				scheduleAt(simTime() + (serviceTime / 10),probe_endServiceMsg);
	}*/
}
