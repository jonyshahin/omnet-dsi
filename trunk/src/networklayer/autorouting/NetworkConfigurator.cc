// Edited By Vincent
//
// Copyright (C) 2006 Andras Varga
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

#include <algorithm>
#include "IRoutingTable.h"
#include "IInterfaceTable.h"
#include "IPAddressResolver.h"
#include "NetworkConfigurator.h"
#include "IPv4InterfaceData.h"


Define_Module(NetworkConfigurator);


void NetworkConfigurator::initialize(int stage)
{
    if (stage==2)
    {
        cTopology topo("topo");
        NodeInfoVector nodeInfo; // will be of size topo.nodes[]

        // extract topology into the cTopology object, then fill in nodeInfo[]
        extractTopology(topo, nodeInfo);

        // assign addresses to IP nodes, and also store result in nodeInfo[].address
        assignAddresses(topo, nodeInfo);

        // add routes for point-to-point peers
        //addPointToPointPeerRoutes(topo, nodeInfo);

        // add default routes to hosts (nodes with a single attachment);
        // also remember result in nodeInfo[].usesDefaultRoute
        addDefaultRoutes(topo, nodeInfo);

        // calculate shortest paths, and add corresponding static routes
        fillRoutingTables(topo, nodeInfo);

        // update display string
        setDisplayString(topo, nodeInfo);
    }
}

void NetworkConfigurator::extractTopology(cTopology& topo, NodeInfoVector& nodeInfo)
{
	int numOfPod, numOfRouter;

    // extract all IP nodes in topology
    topo.extractByProperty("node");
    EV << "cTopology found " << topo.getNumNodes() << " nodes\n";

    // fill in nodeInfo[]
    nodeInfo.resize(topo.getNumNodes());
    for (int i=0; i<topo.getNumNodes(); i++) {
        cModule *mod = topo.getNode(i)->getModule();
        nodeInfo[i].ift = IPAddressResolver().findInterfaceTableOf(mod);
        nodeInfo[i].rt = IPAddressResolver().findRoutingTableOf(mod);
        nodeInfo[i].rt1 = IPAddressResolver().findSecRoutingTableOf(mod);
        nodeInfo[i].isIPNode = nodeInfo[i].rt!=NULL;

        IInterfaceTable *ift = nodeInfo[i].ift;
        // count non-loopback interfaces
        int numIntf = 0;
        InterfaceEntry *ie = NULL;
        for (int k=0; k<ift->getNumInterfaces(); k++) {
        	if (!ift->getInterface(k)->isLoopback()) {
        		ie = ift->getInterface(k);
        		numIntf++;
        	}
        }

        EV << "The number of interfaces are " << ift->getNumInterfaces() << "\n";
        EV << "The number of interfaces are " << numIntf << "\n";

        nodeInfo[i].usesDefaultRoute = (numIntf==1);
        nodeInfo[i].isHost = (numIntf==1);
        nodeInfo[i].isRouter = (numIntf!=1);
        if (nodeInfo[i].isRouter)
        	numOfRouter++;
        // Here I assume that the last node i.e. the node with the highest index in
        // the topology would be a controller. Hence isCotroller is set to true for
        //the last node in the topology.
        if(i == topo.getNumNodes() - 1)
        	nodeInfo[i].isController = true;
    }

    // count number of pods base on number of ports on routers
    // Here it is subtracted by 2 because one of the interface is connected
    // to controller and other one being loopback.
    if (nodeInfo[0].isRouter) {
    	numOfPod = nodeInfo[0].ift->getNumInterfaces()-2;
    }
    EV << "numOfPod = " << numOfPod << " \n";

    // determine whether router is core, aggregate or edge
    // Again since the last node in the topology is Controller it is excluded here
    // while checking for edge or aggregate or core router.
    for (int i=topo.getNumNodes()-2; i>=0; i--) {
    	// node is a router
    	if (nodeInfo[i].isRouter) {
    		cTopology::Node *node = topo.getNode(i);
    		// loop through neighbours
    		for (int j=0; j<node->getNumOutLinks(); j++) {
    			cTopology::Node *neighbour = node->getLinkOut(j)->getRemoteNode();

    			// find neighbour's index in cTopology ==> k
    			int k;
    			for (k=0; k<topo.getNumNodes(); k++)
    				if (topo.getNode(k)==neighbour)
    					break;
    			ASSERT(k<=topo.getNumNodes());

    			// [i] is self, [k] is neighbour
    			// if neighbour is host than node[i] is edge
    			if (nodeInfo[k].isHost)
    				nodeInfo[i].isEdge=true;
    			// if neighbour is edge and node[i] is not host than node[i] is aggregate
    			if (nodeInfo[k].isEdge && !nodeInfo[i].isHost)
    				nodeInfo[i].isAggre=true;
    			// if neighbour is aggregate and node[i] is not edge than node[i] is core
    			if (nodeInfo[k].isAggre && !nodeInfo[i].isEdge)
    				nodeInfo[i].isCore=true;
    		}
    	}
    }

    // determine podId of routers
    for (int i=0; i<topo.getNumNodes(); i++) {

    	// The controller is given a separate pod which is highest in the
    	// The layer id, switch id and the host id are all set to 0.
    	if(nodeInfo[i].isController)
    	{
    		nodeInfo[i].podId = numOfPod + 2;
    		nodeInfo[i].hostId = 0;
    		nodeInfo[i].switchId = 0;
    	}

    	// group all core routers into a pod
    	if (nodeInfo[i].isCore)
    		nodeInfo[i].podId = numOfPod + 1;
    	// determine podId of aggregate
    	if (nodeInfo[i].isAggre) {
    		for (int j=1; j<=numOfPod; j++) {
    			for (int k=0; k<numOfPod/2; k++) {
    				nodeInfo[i].podId=j;
    				i++;
    			}
    		}
    	}
    	// determine podId of edge
    	if (nodeInfo[i].isEdge) {
    		for (int j=1; j<=numOfPod; j++) {
    			for (int k=0; k<numOfPod/2; k++) {
    				nodeInfo[i].podId=j;
    				i++;
    			}
    		}
    	}
    }

    // assign podId of host according to podId of edge
    for (int i=0; i<topo.getNumNodes(); i++) {
    	// assign switchId and hostId of routers
    	if (nodeInfo[i].isRouter && !nodeInfo[i].isController) {
    		nodeInfo[i].switchId=i+1;
    		nodeInfo[i].hostId=0;
    	}
    	if (nodeInfo[i].isEdge) {
    		int l = 1;
			cTopology::Node *node = topo.getNode(i);
			// loop through neighbours
			for (int j=0; j<node->getNumOutLinks(); j++) {
				cTopology::Node *neighbour = node->getLinkOut(j)->getRemoteNode();
				// find neighbour's index in cTopology ==> k
				int k;
				for (k=0; k<topo.getNumNodes(); k++)
					if (topo.getNode(k) == neighbour)
						break;
				ASSERT(k<=topo.getNumNodes());
				// assign podId and hostId to all hosts
				if (nodeInfo[k].isHost) {
					nodeInfo[k].podId = nodeInfo[i].podId;
					nodeInfo[k].switchId = nodeInfo[i].switchId;
					nodeInfo[k].hostId = l++;
				}
        	}
    	}
    }
}

void NetworkConfigurator::assignAddresses(cTopology& topo, NodeInfoVector& nodeInfo)
{
	uint32 podId, layerId, switchId, hostId;

    for (int i=0; i<topo.getNumNodes(); i++)
    {
        // skip bus types
        if (!nodeInfo[i].isIPNode)
            continue;

        // assign podId, layerId, switchId and hostId of addressing scheme
        podId = nodeInfo[i].podId*10 << 24; // left shift by 24 bits -> podId.0.0.0
        // left shift by 16 bits -> 0.layerId.0.0
        if (nodeInfo[i].isCore) {
        	layerId = 3 << 16;
        } else if (nodeInfo[i].isAggre) {
        	layerId = 2 << 16;
        } else if (nodeInfo[i].isEdge) {
        	layerId = 1 << 16;
        } else if (nodeInfo[i].isHost) {
        	layerId = 0 << 16;
        } else if (nodeInfo[i].isController) {
        	layerId = 0 << 16;
        }

        // left shift by 8 bits -> 0.0.switchId.0
        switchId = nodeInfo[i].switchId << 8;
        // -> 0.0.0.hostId
        hostId = nodeInfo[i].hostId;

        // podId.0.0.0 XOR 0.layerId.0.0 XOR 0.0.switchId.0 XOR 0.0.0.hostId -> podId.layerId.switchId.hostId
        uint32 addr = (podId | layerId | switchId | hostId);
        // assign address to all (non-loopback) interfaces
        IInterfaceTable *ift = nodeInfo[i].ift;
        for (int k=0; k<ift->getNumInterfaces(); k++) {
            InterfaceEntry *ie = ift->getInterface(k);
            if (!ie->isLoopback()) {
            	if (nodeInfo[i].isRouter) {
            		ie->ipv4Data()->setIPAddress(IPAddress(addr | (uint) k));
            	} else if (nodeInfo[i].isHost) {
            		ie->ipv4Data()->setIPAddress(IPAddress(addr));
            	}
            	ie->ipv4Data()->setNetmask(IPAddress::ALLONES_ADDRESS); // full address must match for local delivery
            }
        }

        // set routerId as well (if not yet configured)
        IRoutingTable *rt = nodeInfo[i].rt;
        if (rt->getRouterId().isUnspecified()) {
            rt->setRouterId(IPAddress(addr));
        }
        nodeInfo[i].address = (IPAddress(addr));
        cModule *mod = topo.getNode(i)->getModule();
        EV << mod->getName() << " -> ";
        EV << "IP: " << nodeInfo[i].address << " in integer it is " << nodeInfo[i].address.getInt()<<" \n";
    }
}

void NetworkConfigurator::addDefaultRoutes(cTopology& topo, NodeInfoVector& nodeInfo)
{
    // add default route to nodes with exactly one (non-loopback) interface e.g. hosts
    for (int i=0; i<topo.getNumNodes(); i++)
    {
        // skip bus types
        if (!nodeInfo[i].isIPNode)
            continue;

        cTopology::Node *node = topo.getNode(i);
        IInterfaceTable *ift = nodeInfo[i].ift;
        IRoutingTable *rt = nodeInfo[i].rt;

        // count non-loopback interfaces
        int numIntf = 0;
        InterfaceEntry *ie = NULL;
        for (int k=0; k<ift->getNumInterfaces(); k++)
            if (!ift->getInterface(k)->isLoopback())
                {ie = ift->getInterface(k); numIntf++;}

        if (numIntf!=1)
            continue; // skip nodes with more than one (non-loopback) interface

        EV << "  " << node->getModule()->getFullName() << " has only one (non-loopback) "
           "interface, adding default route\n";

        // add default route
        IPRoute *e = new IPRoute();
        e->setNetmask(IPAddress());
        e->setInterface(ie);
        e->setType(IPRoute::REMOTE);
        e->setSource(IPRoute::MANUAL);
        //e->setMetric(1);
        rt->addRoute(e);
    }
}

void NetworkConfigurator::fillRoutingTables(cTopology& topo, NodeInfoVector& nodeInfo)
{
	// fill in routing tables with static routes
	for (int i=0; i<topo.getNumNodes(); i++) {
		cTopology::Node *destNode = topo.getNode(i);

		// skip bus types
		if (!nodeInfo[i].isIPNode)
			continue;

	    IPAddress destAddr = nodeInfo[i].address;
	    std::string destModName = destNode->getModule()->getFullName();

	    // calculate shortest paths from everywhere towards destNode
	    topo.calculateUnweightedSingleShortestPathsTo(destNode);

	    // add route (with host=destNode) to every routing table in the network
	    // (excepting nodes with only one interface -- there we'll set up a default route)
	    for (int j=0; j<topo.getNumNodes(); j++) {
	    	bool direct = false;
	    	if (i==j) continue;
	    	if (!nodeInfo[j].isIPNode)
	    		continue;

	        cTopology::Node *atNode = topo.getNode(j);
	        if (atNode->getNumPaths()==0)
	        	continue; // not connected
	        if (nodeInfo[j].usesDefaultRoute)
	        	continue; // already added default route here

	        // loop through neighbours
	        for (int k=0; k<atNode->getNumOutLinks(); k++) {
	        	cTopology::Node *neighbour = atNode->getLinkOut(k)->getRemoteNode();

	        	if (destNode == neighbour) {
	        		direct = true;
	        	}
	        }
	        // i is neighbour, j is own
	        if ((nodeInfo[i].isAggre && nodeInfo[j].isCore) || (nodeInfo[i].isEdge && nodeInfo[j].isAggre)
	        		|| (nodeInfo[i].isHost && nodeInfo[j].isEdge) || (nodeInfo[i].isCore && nodeInfo[j].isAggre)
	        		|| (nodeInfo[i].isAggre && nodeInfo[j].isEdge)
	        		|| (nodeInfo[i].isAggre && nodeInfo[j].isController) || (nodeInfo[i].isController && nodeInfo[j].isAggre)
	        		|| (nodeInfo[i].isCore && nodeInfo[j].isController) || (nodeInfo[i].isController && nodeInfo[j].isCore)
	        		|| (nodeInfo[i].isEdge && nodeInfo[j].isController) || (nodeInfo[i].isController && nodeInfo[j].isEdge))
	        {
	        	IPAddress atAddr = nodeInfo[j].address;
	        	IInterfaceTable *ift = nodeInfo[j].ift;
	        	IPAddress neighbourAdd[atNode->getNumOutLinks()];
	        	InterfaceEntry *ie1[atNode->getNumOutLinks()];

	        	int outputGateId = atNode->getPath(0)->getLocalGate()->getId();
	        	InterfaceEntry *ie = ift->getInterfaceByNodeOutputGateId(outputGateId);
	        	if (!ie)
	        		error("%s has no interface for output gate id %d", ift->getFullPath().c_str(), outputGateId);

	        	EV << " from " << atNode->getModule()->getFullName() << "=" << IPAddress(atAddr);
	        	EV << " towards " << destModName << "=" << IPAddress(destAddr) << " interface " << ie->getName() << endl;

	        	for (int k=0; k<atNode->getNumOutLinks(); k++) {
	        		ie1[k] = ift->getInterface(k+1);
	        		cTopology::Node *neighbour = atNode->getLinkOut(k)->getRemoteNode();
	        		for (int l=0; l<topo.getNumNodes(); l++) {
	        			cTopology::Node *node = topo.getNode(l);
	        			if (neighbour == node)
	        				neighbourAdd[k] = nodeInfo[l].address;
	        		}
	        	}

	        	if (nodeInfo[j].isCore && nodeInfo[i].isAggre) {
	        		IRoutingTable *rt = nodeInfo[j].rt;
	        		if (direct) {
	        			IPRoute *e = new IPRoute();
	        			e->setHost(destAddr);
	        			e->setNetmask(IPAddress(255,0,0,0)); // only need to match podId
	        			e->setGateway(IPAddress(0,0,0,0));
	        			e->setType(IPRoute::DIRECT);
	        			e->setInterface(ie);
	        			e->setSource(IPRoute::MANUAL);
	        			rt->addRoute(e);
	        		}
	        	} else if (nodeInfo[j].isAggre && nodeInfo[i].isEdge) {
	        		IRoutingTable *rt = nodeInfo[j].rt;
	        		if (direct) {
	        			IPRoute *e = new IPRoute();
	        			e->setHost(destAddr);
	        			e->setNetmask(IPAddress(255,0,255,0)); // only need to match podId and switchId
	        			e->setGateway(IPAddress(0,0,0,0));
	        			e->setType(IPRoute::DIRECT);
	        			e->setInterface(ie);
	        			e->setSource(IPRoute::MANUAL);
	        			rt->addRoute(e);
	        		}
	        	} else if (nodeInfo[j].isAggre && nodeInfo[i].isCore) {
	        		IRoutingTable *rt1 = nodeInfo[j].rt1;
	        		if (direct) {
	        			IPRoute *e = new IPRoute();
	        			e->setHost(destAddr);
	        			e->setNetmask(IPAddress(0,0,0,255)); // routes for multipath selection so doesn't need to match
	        			e->setGateway(IPAddress(0,0,0,0));
	        			e->setType(IPRoute::DIRECT);
	        			e->setInterface(ie);
	        			e->setSource(IPRoute::MANUAL);
	        			rt1->addRoute(e);
	        		}
	        	} else if (nodeInfo[j].isEdge && nodeInfo[i].isHost) {
	        		IRoutingTable *rt = nodeInfo[j].rt;
	        		if (direct) {
	        			IPRoute *e = new IPRoute();
	        			e->setHost(destAddr);
	        			e->setNetmask(IPAddress(255,0,255,255)); // routes to host so full match needed
	        			e->setGateway(IPAddress(0,0,0,0));
	        			e->setType(IPRoute::DIRECT);
	        			e->setInterface(ie);
	        			e->setSource(IPRoute::MANUAL);
	        			rt->addRoute(e);
	        		}
	        	} else if (nodeInfo[j].isEdge && nodeInfo[i].isAggre) {
	        		IRoutingTable *rt1 = nodeInfo[j].rt1;
	        		if (direct) {
	        			IPRoute *e = new IPRoute();
	        			e->setHost(destAddr);
	        			e->setNetmask(IPAddress(0,0,0,255)); // routes for multipath selection so doesn't need to match
	        			e->setGateway(IPAddress(0,0,0,0));
	        			e->setType(IPRoute::DIRECT);
	        			e->setInterface(ie);
	        			e->setSource(IPRoute::MANUAL);
	        			rt1->addRoute(e);
	        		}
	        	} else if (nodeInfo[j].isEdge && nodeInfo[i].isController) {
	        		IRoutingTable *rt = nodeInfo[j].rt;
	        		if (direct) {
	        			IPRoute *e = new IPRoute();
	        			e->setHost(destAddr);
	        			e->setNetmask(IPAddress(255,0,0,0)); // routes for multipath selection so doesn't need to match
	        			e->setGateway(IPAddress(0,0,0,0));
	        			e->setType(IPRoute::DIRECT);
	        			e->setInterface(ie);
	        			e->setSource(IPRoute::MANUAL);
	        			rt->addRoute(e);
	        		}
	        	} else if (nodeInfo[j].isController && nodeInfo[i].isEdge) {
	        		IRoutingTable *rt = nodeInfo[j].rt;
	        		if (direct) {
	        			IPRoute *e = new IPRoute();
	        			e->setHost(destAddr);
	        			e->setNetmask(IPAddress(255,255,255,255)); // Complete match is needed in the routing table of the controller
	        			e->setGateway(IPAddress(0,0,0,0));
	        			e->setType(IPRoute::DIRECT);
	        			e->setInterface(ie);
	        			e->setSource(IPRoute::MANUAL);
	        			rt->addRoute(e);
	        		}
	        	} else if (nodeInfo[j].isAggre && nodeInfo[i].isController) {
	        		IRoutingTable *rt = nodeInfo[j].rt;
	        		if (direct) {
	        			IPRoute *e = new IPRoute();
	        			e->setHost(destAddr);
	        			e->setNetmask(IPAddress(255,0,0,0)); // routes for multipath selection so doesn't need to match
	        			e->setGateway(IPAddress(0,0,0,0));
	        			e->setType(IPRoute::DIRECT);
	        			e->setInterface(ie);
	        			e->setSource(IPRoute::MANUAL);
	        			rt->addRoute(e);
	        		}
	        	} else if (nodeInfo[j].isController && nodeInfo[i].isAggre) {
	        		IRoutingTable *rt = nodeInfo[j].rt;
	        		if (direct) {
	        			IPRoute *e = new IPRoute();
	        			e->setHost(destAddr);
	        			e->setNetmask(IPAddress(255,255,255,255)); // Complete match is needed in the routing table of the controller
	        			e->setGateway(IPAddress(0,0,0,0));
	        			e->setType(IPRoute::DIRECT);
	        			e->setInterface(ie);
	        			e->setSource(IPRoute::MANUAL);
	        			rt->addRoute(e);
	        		}
	        	} else if (nodeInfo[j].isCore && nodeInfo[i].isController) {
	        		IRoutingTable *rt = nodeInfo[j].rt;
	        		if (direct) {
	        			IPRoute *e = new IPRoute();
	        			e->setHost(destAddr);
	        			e->setNetmask(IPAddress(255,0,0,0)); // routes for multipath selection so doesn't need to match
	        			e->setGateway(IPAddress(0,0,0,0));
	        			e->setType(IPRoute::DIRECT);
	        			e->setInterface(ie);
	        			e->setSource(IPRoute::MANUAL);
	        			rt->addRoute(e);
	        		}
	        	} else if (nodeInfo[j].isController && nodeInfo[i].isCore) {
	        		IRoutingTable *rt = nodeInfo[j].rt;
	        		if (direct) {
	        			IPRoute *e = new IPRoute();
	        			e->setHost(destAddr);
	        			e->setNetmask(IPAddress(255,255,255,255)); // Complete match is needed in the routing table of the controller
	        			e->setGateway(IPAddress(0,0,0,0));
	        			e->setType(IPRoute::DIRECT);
	        			e->setInterface(ie);
	        			e->setSource(IPRoute::MANUAL);
	        			rt->addRoute(e);
	        		}
	        	}
	        }
        }
    }
}

void NetworkConfigurator::handleMessage(cMessage *msg)
{
    error("this module doesn't handle messages, it runs only in initialize()");
}

void NetworkConfigurator::setDisplayString(cTopology& topo, NodeInfoVector& nodeInfo)
{
    int numIPNodes=0, numHost=0, numCore=0, numAggre=0, numEdge=0;
    // calculate number of nodes, hosts, core, aggregate and edge
    for (int i=0; i<topo.getNumNodes(); i++) {
        if (nodeInfo[i].isIPNode)
            numIPNodes++;
    	if (nodeInfo[i].isHost)
    		numHost++;
    	if (nodeInfo[i].isCore)
    		numCore++;
    	if (nodeInfo[i].isAggre)
    		numAggre++;
    	if (nodeInfo[i].isEdge)
    		numEdge++;
    }

    // update display string
    char buf[80];
    sprintf(buf, "%d IP nodes\n%d non-IP nodes\n%d cores\n%d aggregates\n%d edges\n%d hosts", numIPNodes, topo.getNumNodes()-numIPNodes,
    		numCore, numAggre, numEdge, numHost);
    getDisplayString().setTagArg("t",0,buf);
}


