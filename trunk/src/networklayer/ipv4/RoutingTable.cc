//
// Copyright (C) 2004-2006 Andras Varga
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
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


//  Cleanup and rewrite: Andras Varga, 2004

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <algorithm>
#include <sstream>
#include <iostream>
#include "sha1.h"

#include "RoutingTable.h"
#include "RoutingTableParser.h"
#include "IPRoute.h"
#include "IPv4InterfaceData.h"
#include "IInterfaceTable.h"
#include "InterfaceTableAccess.h"
#include "NotifierConsts.h"


Define_Module(RoutingTable);


std::ostream& operator<<(std::ostream& os, const IPRoute& e)
{
    os << e.info();
    return os;
};

RoutingTable::RoutingTable()
{
}

RoutingTable::~RoutingTable()
{
    for (unsigned int i=0; i<routes.size(); i++)
        delete routes[i];
    for (unsigned int i=0; i<multicastRoutes.size(); i++)
        delete multicastRoutes[i];
}

void RoutingTable::initialize(int stage)
{
    if (stage==0)
    {
        // get a pointer to the NotificationBoard module and IInterfaceTable
        nb = NotificationBoardAccess().get();
        ift = InterfaceTableAccess().get();

        IPForward = par("IPForward").boolValue();

        nb->subscribe(this, NF_INTERFACE_CREATED);
        nb->subscribe(this, NF_INTERFACE_DELETED);
        nb->subscribe(this, NF_INTERFACE_STATE_CHANGED);
        nb->subscribe(this, NF_INTERFACE_CONFIG_CHANGED);
        nb->subscribe(this, NF_INTERFACE_IPv4CONFIG_CHANGED);

        WATCH_PTRVECTOR(routes);
        WATCH_PTRVECTOR(multicastRoutes);
        WATCH(IPForward);
        WATCH(routerId);
    }
    else if (stage==1)
    {
        // L2 modules register themselves in stage 0, so we can only configure
        // the interfaces in stage 1.
        const char *filename = par("routingFile");

        // At this point, all L2 modules have registered themselves (added their
        // interface entries). Create the per-interface IPv4 data structures.
        IInterfaceTable *interfaceTable = InterfaceTableAccess().get();
        for (int i=0; i<interfaceTable->getNumInterfaces(); ++i)
            configureInterfaceForIPv4(interfaceTable->getInterface(i));
        configureLoopbackForIPv4();

        // read routing table file (and interface configuration)
        RoutingTableParser parser(ift, this);
        if (*filename && parser.readRoutingTableFromFile(filename)==-1)
            error("Error reading routing table file %s", filename);

        // set routerId if param is not "" (==no routerId) or "auto" (in which case we'll
        // do it later in stage 3, after network configurators configured the interfaces)
        const char *routerIdStr = par("routerId").stringValue();
        if (strcmp(routerIdStr, "") && strcmp(routerIdStr, "auto"))
            routerId = IPAddress(routerIdStr);
    }
    else if (stage==3)
    {
        // routerID selection must be after stage==2 when network autoconfiguration
        // assigns interface addresses
        configureRouterId();

        // we don't use notifications during initialize(), so we do it manually.
        // Should be in stage=3 because autoconfigurator runs in stage=2.
        updateNetmaskRoutes();

        //printRoutingTable();
    }
}

void RoutingTable::configureRouterId()
{
    if (routerId.isUnspecified())  // not yet configured
    {
        const char *routerIdStr = par("routerId").stringValue();
        if (!strcmp(routerIdStr, "auto"))  // non-"auto" cases already handled in stage 1
        {
            // choose highest interface address as routerId
            for (int i=0; i<ift->getNumInterfaces(); ++i)
            {
                InterfaceEntry *ie = ift->getInterface(i);
                if (!ie->isLoopback() && ie->ipv4Data()->getIPAddress().getInt() > routerId.getInt())
                    routerId = ie->ipv4Data()->getIPAddress();
            }
        }
    }
    else // already configured
    {
        // if there is no interface with routerId yet, assign it to the loopback address;
        // TODO find out if this is a good practice, in which situations it is useful etc.
        if (getInterfaceByAddress(routerId)==NULL)
        {
            InterfaceEntry *lo0 = ift->getFirstLoopbackInterface();
            lo0->ipv4Data()->setIPAddress(routerId);
            lo0->ipv4Data()->setNetmask(IPAddress::ALLONES_ADDRESS);
        }
    }
}

void RoutingTable::updateDisplayString()
{
    if (!ev.isGUI())
        return;

    char buf[80];
    if (routerId.isUnspecified())
        sprintf(buf, "%d+%d routes", routes.size(), multicastRoutes.size());
    else
        sprintf(buf, "routerId: %s\n%d+%d routes", routerId.str().c_str(), routes.size(), multicastRoutes.size());
    getDisplayString().setTagArg("t",0,buf);
}

void RoutingTable::handleMessage(cMessage *msg)
{
    opp_error("This module doesn't process messages");
}

void RoutingTable::receiveChangeNotification(int category, const cPolymorphic *details)
{
    if (simulation.getContextType()==CTX_INITIALIZE)
        return;  // ignore notifications during initialize

    Enter_Method_Silent();
    printNotificationBanner(category, details);

    if (category==NF_INTERFACE_CREATED)
    {
        // add netmask route for the new interface
        updateNetmaskRoutes();
    }
    else if (category==NF_INTERFACE_DELETED)
    {
        // remove all routes that point to that interface
        InterfaceEntry *entry = check_and_cast<InterfaceEntry*>(details);
        deleteInterfaceRoutes(entry);
    }
    else if (category==NF_INTERFACE_STATE_CHANGED)
    {
        invalidateCache();
    }
    else if (category==NF_INTERFACE_CONFIG_CHANGED)
    {
        invalidateCache();
    }
    else if (category==NF_INTERFACE_IPv4CONFIG_CHANGED)
    {
        // if anything IPv4-related changes in the interfaces, interface netmask
        // based routes have to be re-built.
        updateNetmaskRoutes();
    }
}

void RoutingTable::deleteInterfaceRoutes(InterfaceEntry *entry)
{
    RouteVector::iterator it = routes.begin();
    while (it != routes.end())
    {
        IPRoute *route = *it;
        if (route->getInterface() == entry)
        {
            deleteRoute(route);
            it = routes.begin();  // iterator became invalid -- start over
        }
        else
        {
            ++it;
        }
    }
}

void RoutingTable::invalidateCache()
{
    routingCache.clear();
    localAddresses.clear();
}

void RoutingTable::printRoutingTable() const
{
    EV << "-- Routing table --\n";
    ev.printf("%-16s %-16s %-16s %-3s %s\n",
              "Destination", "Gateway", "Netmask", "Iface");

    for (int i=0; i<getNumRoutes(); i++)
        EV << getRoute(i)->detailedInfo() << "\n";
    EV << "\n";
}

std::vector<IPAddress> RoutingTable::gatherAddresses() const
{
    std::vector<IPAddress> addressvector;

    for (int i=0; i<ift->getNumInterfaces(); ++i)
        addressvector.push_back(ift->getInterface(i)->ipv4Data()->getIPAddress());
    return addressvector;
}

//---

void RoutingTable::configureInterfaceForIPv4(InterfaceEntry *ie)
{
    IPv4InterfaceData *d = new IPv4InterfaceData();
    ie->setIPv4Data(d);

    // metric: some hints: OSPF cost (2e9/bps value), MS KB article Q299540, ...
    d->setMetric((int)ceil(2e9/ie->getDatarate())); // use OSPF cost as default
}

InterfaceEntry *RoutingTable::getInterfaceByAddress(const IPAddress& addr) const
{
    Enter_Method("getInterfaceByAddress(%x)", addr.getInt()); // note: str().c_str() too slow here

    if (addr.isUnspecified())
        return NULL;
    for (int i=0; i<ift->getNumInterfaces(); ++i)
    {
        InterfaceEntry *ie = ift->getInterface(i);
        if (ie->ipv4Data()->getIPAddress()==addr)
            return ie;
    }
    return NULL;
}


void RoutingTable::configureLoopbackForIPv4()
{
    InterfaceEntry *ie = ift->getFirstLoopbackInterface();

    // add IPv4 info. Set 127.0.0.1/8 as address by default --
    // we may reconfigure later it to be the routerId
    IPv4InterfaceData *d = new IPv4InterfaceData();
    d->setIPAddress(IPAddress::LOOPBACK_ADDRESS);
    d->setNetmask(IPAddress::LOOPBACK_NETMASK);
    d->setMetric(1);
    ie->setIPv4Data(d);
}

//---

bool RoutingTable::isLocalAddress(const IPAddress& dest) const
{
    Enter_Method("isLocalAddress(%x)", dest.getInt()); // note: str().c_str() too slow here

    if (localAddresses.empty())
    {
    	localAddresses.insert(routerId);
    }

    AddressSet::iterator it = localAddresses.find(dest);
    return it!=localAddresses.end();
}

bool RoutingTable::isLocalMulticastAddress(const IPAddress& dest) const
{
    Enter_Method("isLocalMulticastAddress(%x)", dest.getInt()); // note: str().c_str() too slow here

    for (int i=0; i<ift->getNumInterfaces(); i++)
    {
        InterfaceEntry *ie = ift->getInterface(i);
        if (ie->ipv4Data()->isMemberOfMulticastGroup(dest))
            return true;
    }
    return false;
}

const IPRoute *RoutingTable::hashRouting(int dest, int src, int destPort, int srcPort) const
{
    Enter_Method("multipathRoute(%x)", dest); // note: str().c_str() too slow here

    ev << dest << " " << src << "\n";
    // Variables for secure hashing
    SHA1 sha;
    char buff[32];
    unsigned message_digest[5];

    sha.Reset(); // Clear previous hash contents

    // Hash destIP, srcIP, destPort and srcPort
    itoa(dest, buff, 16); // Convert int into char array before hashing
    sha.Input(buff, 32); // Hash input
    itoa(src, buff, 16);
    sha.Input(buff, 32);
    itoa(destPort, buff, 16);
    sha.Input(buff, 32);
    itoa(srcPort, buff, 16);
    sha.Input(buff, 32);

    sha.Result(message_digest); // Conclude hashing. Results will be stored in an unsigned int array of 5 elements -> message_digest[5]

    // Variables for converting hash results into char array of 160-bits
    char hexa[8];
    char temp[40];
    char test[40][4];
    char digest[160];

    for (int i=0; i<5; i++) {
    	// Ensure that all SHA1 results will generate 8 hexadecimal char
    	while (message_digest[i] < 268435456) { // 268435456 in decimal == 10000000 in hexadecimal
    		message_digest[i] += message_digest[i];
    	}
    	ev << "message_digest[" << i << "]: " << message_digest[i] << "\n"; // Print SHA1 results
    	itoa(message_digest[i], hexa, 16); // Convert SHA1 results into char array of 8 hexadecimal char -> hexa[8]
    	ev << "hexadecimal representation: " << hexa << "\n"; // Verify that the char array of 8 hexadecimal char is the correct representation of SHA1 results
    	// Combine the 5 hexa[8] into a char array of 40 hexadecimal char -> temp[40]
    	int k=7;
    	for (int j=0; j<8; j++) {
    		temp[k+(i*8)] = hexa[j];
    		k--;
    	}
    }

    // Convert hexadecimal char of temp[40] into their respective binary representation -> test[40][4]
    for (int i=39; i>=0; i--) {
    	itoa(temp[i], test[i], 2);
    	if (temp[i] == '0') {
    		test[i][0] = '0'; test[i][1] = '0'; test[i][2] = '0'; test[i][3] = '0';
    	} else if (temp[i] == '1') {
    		test[i][0] = '0'; test[i][1] = '0'; test[i][2] = '0'; test[i][3] = '1';
    	} else if (temp[i] == '2') {
    		test[i][0] = '0'; test[i][1] = '0'; test[i][2] = '1'; test[i][3] = '0';
    	} else if (temp[i] == '3') {
    		test[i][0] = '0'; test[i][1] = '0'; test[i][2] = '1'; test[i][3] = '1';
    	} else if (temp[i] == '4') {
    		test[i][0] = '0'; test[i][1] = '1'; test[i][2] = '0'; test[i][3] = '0';
    	} else if (temp[i] == '5') {
    		test[i][0] = '0'; test[i][1] = '1'; test[i][2] = '0'; test[i][3] = '1';
    	} else if (temp[i] == '6') {
    		test[i][0] = '0'; test[i][1] = '1'; test[i][2] = '1'; test[i][3] = '0';
    	} else if (temp[i] == '7') {
    		test[i][0] = '0'; test[i][1] = '1'; test[i][2] = '1'; test[i][3] = '1';
    	} else if (temp[i] == '8') {
    		test[i][0] = '1'; test[i][1] = '0'; test[i][2] = '0'; test[i][3] = '0';
    	} else if (temp[i] == '9') {
    		test[i][0] = '1'; test[i][1] = '0'; test[i][2] = '0'; test[i][3] = '1';
    	} else if (temp[i] == 'a') {
    		test[i][0] = '1'; test[i][1] = '0'; test[i][2] = '1'; test[i][3] = '0';
    	} else if (temp[i] == 'b') {
    		test[i][0] = '1'; test[i][1] = '0'; test[i][2] = '1'; test[i][3] = '1';
    	} else if (temp[i] == 'c') {
    		test[i][0] = '1'; test[i][1] = '1'; test[i][2] = '0'; test[i][3] = '0';
    	} else if (temp[i] == 'd') {
    		test[i][0] = '1'; test[i][1] = '1'; test[i][2] = '0'; test[i][3] = '1';
    	} else if (temp[i] == 'e') {
    		test[i][0] = '1'; test[i][1] = '1'; test[i][2] = '1'; test[i][3] = '0';
    	} else if (temp[i] == 'f') {
    		test[i][0] = '1'; test[i][1] = '1'; test[i][2] = '1'; test[i][3] = '1';
    	}
    	ev << temp[i];

    	// Convert test[40][4] into a single char array of 160 binary
    	int k=3;
    	for (int j=0; j<4; j++) {
    		digest[k+(i*4)] = test[i][j];
    		k--;
    	}
    }
    ev << "\n";
    for (int l=159; l>=0; l--) {
    	ev << digest[l];
    }
    ev << "\n";

    // multipath selection algorthim
    // work up to routers with 64 ports i.e 32 multipath connections
    const IPRoute *bestRoute = NULL;
    int numCon = (ift->getNumInterfaces() - 2)/2; // Subtract it by 2 here because one of them is loopback and the other being link to controller which is also excluded.
    int windowSize = 0;
    int numWindow = 0;

    // determine size of each window based on the number of interface
    if (numCon == 2) {
    	windowSize = 1;
   	} else if (numCon > 2 && numCon < 5) {
   		windowSize = 2;
   	} else if (numCon > 4 && numCon < 9) {
   		windowSize = 3;
   	} else if (numCon > 8 && numCon < 17) {
   		windowSize = 4;
   	} else if (numCon > 16 && numCon < 33) {
   		windowSize = 5;
   	}

    numWindow = 160/windowSize; // number of windows available
    char window[numWindow][windowSize];
    int value;

    // break down SHA1 results into windows
    int l=159;
    for (int i=0; i<numWindow; i++) {
    	for (int j=0; j<windowSize; j++) {
    		window[i][j] = digest[l];
    		l--;
    	}
    }

    // get correspond decimal value from window of binary numbers
    int i = 0;
    do {
    	if (windowSize == 1) {
    		if (window[i][0] == '0') {
    			value = 0;
    		} else if (window[i][0] == '1') {
    			value = 1;
    	    }
    	} else if (windowSize == 2) {
   		 	if (window[i][1] == '0' && window[i][0] == '0') {
   		 		value = 0;
   		 	} else if (window[i][1] == '0' && window[i][0] == '1') {
   		 		value = 1;
   		 	} else if (window[i][1] == '1' && window[i][0] == '0') {
   		 		value = 2;
   		 	} else if (window[i][1] == '1' && window[i][0] == '1') {
   		 		value = 3;
   		 	}
   		} else if (windowSize == 3) {
   			if (window[i][2] == '0' && window[i][1] == '0' && window[i][0] == '0') {
   				value = 0;
   			} else if (window[i][2] == '0' && window[i][1] == '0' && window[i][0] == '1') {
   				value = 1;
   			} else if (window[i][2] == '0' && window[i][1] == '1' && window[i][0] == '0') {
   				value = 2;
   			} else if (window[i][2] == '0' && window[i][1] == '1' && window[i][0] == '1') {
   				value = 3;
   			} else if (window[i][2] == '1' && window[i][1] == '0' && window[i][0] == '0') {
   				value = 4;
   			} else if (window[i][2] == '1' && window[i][1] == '0' && window[i][0] == '1') {
   				value = 5;
   			} else if (window[i][2] == '1' && window[i][1] == '1' && window[i][0] == '0') {
   				value = 6;
   			} else if (window[i][2] == '1' && window[i][1] == '1' && window[i][0] == '1') {
   				value = 7;
   			}
   		} else if (windowSize == 4) {
   			if (window[i][3] == '0' && window[i][2] == '0' && window[i][1] == '0' && window[i][0] == '0') {
   				value = 0;
   			} else if (window[i][3] == '0' && window[i][2] == '0' && window[i][1] == '0' && window[i][0] == '1') {
   				value = 1;
   			} else if (window[i][3] == '0' && window[i][2] == '0' && window[i][1] == '1' && window[i][0] == '0') {
   				value = 2;
   			} else if (window[i][3] == '0' && window[i][2] == '0' && window[i][1] == '1' && window[i][0] == '1') {
   				value = 3;
   			} else if (window[i][3] == '0' && window[i][2] == '1' && window[i][1] == '0' && window[i][0] == '0') {
   				value = 4;
   			} else if (window[i][3] == '0' && window[i][2] == '1' && window[i][1] == '0' && window[i][0] == '1') {
   				value = 5;
   			} else if (window[i][3] == '0' && window[i][2] == '1' && window[i][1] == '1' && window[i][0] == '0') {
   				value = 6;
   			} else if (window[i][3] == '0' && window[i][2] == '1' && window[i][1] == '1' && window[i][0] == '1') {
   				value = 7;
   			} else if (window[i][3] == '1' && window[i][2] == '0' && window[i][1] == '0' && window[i][0] == '0') {
   				value = 8;
   			} else if (window[i][3] == '1' && window[i][2] == '0' && window[i][1] == '0' && window[i][0] == '1') {
   				value = 9;
   			} else if (window[i][3] == '1' && window[i][2] == '0' && window[i][1] == '1' && window[i][0] == '0') {
   				value = 10;
   			} else if (window[i][3] == '1' && window[i][2] == '0' && window[i][1] == '1' && window[i][0] == '1') {
   				value = 11;
   			} else if (window[i][3] == '1' && window[i][2] == '1' && window[i][1] == '0' && window[i][0] == '0') {
   				value = 12;
   			} else if (window[i][3] == '1' && window[i][2] == '1' && window[i][1] == '0' && window[i][0] == '1') {
   				value = 13;
   			} else if (window[i][3] == '1' && window[i][2] == '1' && window[i][1] == '1' && window[i][0] == '0') {
   				value = 14;
   			} else if (window[i][3] == '1' && window[i][2] == '1' && window[i][1] == '1' && window[i][0] == '1') {
   				value = 15;
   			}
   		} else if (windowSize == 5) {
   			if (window[i][4] == '0' && window[i][3] == '0' && window[i][2] == '0' && window[i][1] == '0' && window[i][0] == '0') {
   				value = 0;
   			} else if (window[i][4] == '0' && window[i][3] == '0' && window[i][2] == '0' && window[i][1] == '0' && window[i][0] == '1') {
   				value = 1;
   			} else if (window[i][4] == '0' && window[i][3] == '0' && window[i][2] == '0' && window[i][1] == '1' && window[i][0] == '0') {
   				value = 2;
   			} else if (window[i][4] == '0' && window[i][3] == '0' && window[i][2] == '0' && window[i][1] == '1' && window[i][0] == '1') {
   				value = 3;
   			} else if (window[i][4] == '0' && window[i][3] == '0' && window[i][2] == '1' && window[i][1] == '0' && window[i][0] == '0') {
   				value = 4;
   			} else if (window[i][4] == '0' && window[i][3] == '0' && window[i][2] == '1' && window[i][1] == '0' && window[i][0] == '1') {
   				value = 5;
   			} else if (window[i][4] == '0' && window[i][3] == '0' && window[i][2] == '1' && window[i][1] == '1' && window[i][0] == '0') {
   				value = 6;
   			} else if (window[i][4] == '0' && window[i][3] == '0' && window[i][2] == '1' && window[i][1] == '1' && window[i][0] == '1') {
   				value = 7;
   			} else if (window[i][4] == '0' && window[i][3] == '1' && window[i][2] == '0' && window[i][1] == '0' && window[i][0] == '0') {
   				value = 8;
   			} else if (window[i][4] == '0' && window[i][3] == '1' && window[i][2] == '0' && window[i][1] == '0' && window[i][0] == '1') {
   				value = 9;
   			} else if (window[i][4] == '0' && window[i][3] == '1' && window[i][2] == '0' && window[i][1] == '1' && window[i][0] == '0') {
   				value = 10;
   			} else if (window[i][4] == '0' && window[i][3] == '1' && window[i][2] == '0' && window[i][1] == '1' && window[i][0] == '1') {
   				value = 11;
   			} else if (window[i][4] == '0' && window[i][3] == '1' && window[i][2] == '1' && window[i][1] == '0' && window[i][0] == '0') {
   				value = 12;
   			} else if (window[i][4] == '0' && window[i][3] == '1' && window[i][2] == '1' && window[i][1] == '0' && window[i][0] == '1') {
   				value = 13;
   			} else if (window[i][4] == '0' && window[i][3] == '1' && window[i][2] == '1' && window[i][1] == '1' && window[i][0] == '0') {
   				value = 14;
   			} else if (window[i][4] == '0' && window[i][3] == '1' && window[i][2] == '1' && window[i][1] == '1' && window[i][0] == '1') {
   				value = 15;
   			} else if (window[i][4] == '1' && window[i][3] == '0' && window[i][2] == '0' && window[i][1] == '0' && window[i][0] == '0') {
   				value = 16;
   			} else if (window[i][4] == '1' && window[i][3] == '0' && window[i][2] == '0' && window[i][1] == '0' && window[i][0] == '1') {
   				value = 17;
   			} else if (window[i][4] == '1' && window[i][3] == '0' && window[i][2] == '0' && window[i][1] == '1' && window[i][0] == '0') {
   				value = 18;
   			} else if (window[i][4] == '1' && window[i][3] == '0' && window[i][2] == '0' && window[i][1] == '1' && window[i][0] == '1') {
   				value = 19;
   			} else if (window[i][4] == '1' && window[i][3] == '0' && window[i][2] == '1' && window[i][1] == '0' && window[i][0] == '0') {
   				value = 20;
   			} else if (window[i][4] == '1' && window[i][3] == '0' && window[i][2] == '1' && window[i][1] == '0' && window[i][0] == '1') {
   				value = 21;
   			} else if (window[i][4] == '1' && window[i][3] == '0' && window[i][2] == '1' && window[i][1] == '1' && window[i][0] == '0') {
   				value = 22;
   			} else if (window[i][4] == '1' && window[i][3] == '0' && window[i][2] == '1' && window[i][1] == '1' && window[i][0] == '1') {
   				value = 23;
   			} else if (window[i][4] == '1' && window[i][3] == '1' && window[i][2] == '0' && window[i][1] == '0' && window[i][0] == '0') {
   				value = 24;
   			} else if (window[i][4] == '1' && window[i][3] == '1' && window[i][2] == '0' && window[i][1] == '0' && window[i][0] == '1') {
   				value = 25;
   			} else if (window[i][4] == '1' && window[i][3] == '1' && window[i][2] == '0' && window[i][1] == '1' && window[i][0] == '0') {
   				value = 26;
   			} else if (window[i][4] == '1' && window[i][3] == '1' && window[i][2] == '0' && window[i][1] == '1' && window[i][0] == '1') {
   				value = 27;
   			} else if (window[i][4] == '1' && window[i][3] == '1' && window[i][2] == '1' && window[i][1] == '0' && window[i][0] == '0') {
   				value = 28;
   			} else if (window[i][4] == '1' && window[i][3] == '1' && window[i][2] == '1' && window[i][1] == '0' && window[i][0] == '1') {
   				value = 29;
   			} else if (window[i][4] == '1' && window[i][3] == '1' && window[i][2] == '1' && window[i][1] == '1' && window[i][0] == '0') {
   				value = 30;
   			} else if (window[i][4] == '1' && window[i][3] == '1' && window[i][2] == '1' && window[i][1] == '1' && window[i][0] == '1') {
   				value = 31;
   			}
   		}
    	// equivalent to negating MSB of last window
   		if ((i == (numWindow-1)) && (value>numCon)) {
   			value = value-pow(2,windowSize-1);
   		}
   		i++;
   	} while (value>=numCon);
    bestRoute = getRoute(value);
    return bestRoute;
}

const IPRoute *RoutingTable::findBestMatchingRoute(const IPAddress& dest) const
{
    Enter_Method("findBestMatchingRoute(%x)", dest.getInt()); // note: str().c_str() too slow here
    EV << "The best route to be found" << "\n";
    RoutingCache::iterator it = routingCache.find(dest);
    if (it != routingCache.end())
        return it->second;

    // find best match (one with longest prefix)
    // default route has zero prefix length, so (if exists) it'll be selected as last resort
    const IPRoute *bestRoute = NULL;
    uint32 longestNetmask = 0;
    for (RouteVector::const_iterator i=routes.begin(); i!=routes.end(); ++i)
    {
        const IPRoute *e = *i;
        if (IPAddress::maskedAddrAreEqual(dest, e->getHost(), e->getNetmask()) &&  // match
            (!bestRoute || e->getNetmask().getInt() > longestNetmask))  // longest so far
        {
            bestRoute = e;
            longestNetmask = e->getNetmask().getInt();
        }
    }
    routingCache[dest] = bestRoute;
    EV << "The best route is " << bestRoute << "\n";
    return bestRoute;
}

InterfaceEntry *RoutingTable::getInterfaceForDestAddr(const IPAddress& dest) const
{
    Enter_Method("getInterfaceForDestAddr(%x)", dest.getInt()); // note: str().c_str() too slow here

    const IPRoute *e = findBestMatchingRoute(dest);
    return e ? e->getInterface() : NULL;
}

IPAddress RoutingTable::getGatewayForDestAddr(const IPAddress& dest) const
{
    Enter_Method("getGatewayForDestAddr(%x)", dest.getInt()); // note: str().c_str() too slow here

    const IPRoute *e = findBestMatchingRoute(dest);
    return e ? e->getGateway() : IPAddress();
}


MulticastRoutes RoutingTable::getMulticastRoutesFor(const IPAddress& dest) const
{
    Enter_Method("getMulticastRoutesFor(%x)", dest.getInt()); // note: str().c_str() too slow here here

    MulticastRoutes res;
    res.reserve(16);
    for (RouteVector::const_iterator i=multicastRoutes.begin(); i!=multicastRoutes.end(); ++i)
    {
        const IPRoute *e = *i;
        if (IPAddress::maskedAddrAreEqual(dest, e->getHost(), e->getNetmask()))
        {
            MulticastRoute r;
            r.interf = e->getInterface();
            r.gateway = e->getGateway();
            res.push_back(r);
        }
    }
    return res;
}


int RoutingTable::getNumRoutes() const
{
    return routes.size()+multicastRoutes.size();
}

const IPRoute *RoutingTable::getRoute(int k) const
{
    if (k < (int)routes.size())
        return routes[k];
    k -= routes.size();
    if (k < (int)multicastRoutes.size())
        return multicastRoutes[k];
    return NULL;
}

const IPRoute *RoutingTable::getDefaultRoute() const
{
    int n = (int)routes.size();
    for (int i=0; i<n; i++)
        if (routes[i]->getNetmask().isUnspecified())
            return routes[i];
    return NULL;
}

const IPRoute *RoutingTable::findRoute(const IPAddress& target, const IPAddress& netmask,
    const IPAddress& gw, int metric, const char *dev) const
{
    int n = getNumRoutes();
    for (int i=0; i<n; i++)
        if (routeMatches(getRoute(i), target, netmask, gw, metric, dev))
            return getRoute(i);
    return NULL;
}

void RoutingTable::addRoute(const IPRoute *entry)
{
    Enter_Method("addRoute(...)");

    // check for null address and default route
    if (entry->getHost().isUnspecified() != entry->getNetmask().isUnspecified())
        error("addRoute(): to add a default route, set both host and netmask to zero");

    if (entry->getHost().doAnd(entry->getNetmask().isUnspecified()).getInt() != 0)
        error("addRoute(): suspicious route: host %s has 1-bits outside netmask %s",
              entry->getHost().str().c_str(), entry->getNetmask().str().c_str());

    // check that the interface exists
    if (!entry->getInterface())
        error("addRoute(): interface cannot be NULL");

    // if this is a default route, remove old default route (we're replacing it)
    if (entry->getNetmask().isUnspecified() && getDefaultRoute()!=NULL)
        deleteRoute(getDefaultRoute());

    // add to tables
    if (!entry->getHost().isMulticast())
        routes.push_back(const_cast<IPRoute*>(entry));
    else
        multicastRoutes.push_back(const_cast<IPRoute*>(entry));

    invalidateCache();
    updateDisplayString();

    nb->fireChangeNotification(NF_IPv4_ROUTE_ADDED, entry);
}


bool RoutingTable::deleteRoute(const IPRoute *entry)
{
    Enter_Method("deleteRoute(...)");

    RouteVector::iterator i = std::find(routes.begin(), routes.end(), entry);
    if (i!=routes.end())
    {
        nb->fireChangeNotification(NF_IPv4_ROUTE_DELETED, entry); // rather: going to be deleted
        routes.erase(i);
        delete entry;
        invalidateCache();
        updateDisplayString();
        return true;
    }
    i = std::find(multicastRoutes.begin(), multicastRoutes.end(), entry);
    if (i!=multicastRoutes.end())
    {
        nb->fireChangeNotification(NF_IPv4_ROUTE_DELETED, entry); // rather: going to be deleted
        multicastRoutes.erase(i);
        delete entry;
        invalidateCache();
        updateDisplayString();
        return true;
    }
    return false;
}


bool RoutingTable::routeMatches(const IPRoute *entry,
    const IPAddress& target, const IPAddress& nmask,
    const IPAddress& gw, int metric, const char *dev) const
{
    if (!target.isUnspecified() && !target.equals(entry->getHost()))
        return false;
    if (!nmask.isUnspecified() && !nmask.equals(entry->getNetmask()))
        return false;
    if (!gw.isUnspecified() && !gw.equals(entry->getGateway()))
        return false;
    if (metric && metric!=entry->getMetric())
        return false;
    if (dev && strcmp(dev, entry->getInterfaceName()))
        return false;

    return true;
}

void RoutingTable::updateNetmaskRoutes()
{
    // first, delete all routes with src=IFACENETMASK
    for (unsigned int k=0; k<routes.size(); k++)
        if (routes[k]->getSource()==IPRoute::IFACENETMASK)
            routes.erase(routes.begin()+(k--));  // '--' is necessary because indices shift down

    // then re-add them, according to actual interface configuration
    for (int i=0; i<ift->getNumInterfaces(); i++)
    {
        InterfaceEntry *ie = ift->getInterface(i);
        if (ie->ipv4Data()->getNetmask()!=IPAddress::ALLONES_ADDRESS)
        {
            IPRoute *route = new IPRoute();
            route->setType(IPRoute::DIRECT);
            route->setSource(IPRoute::IFACENETMASK);
            route->setHost(ie->ipv4Data()->getIPAddress());
            route->setNetmask(ie->ipv4Data()->getNetmask());
            route->setGateway(IPAddress());
            route->setMetric(ie->ipv4Data()->getMetric());
            route->setInterface(ie);
            routes.push_back(route);
        }
    }

    invalidateCache();
    updateDisplayString();
}


