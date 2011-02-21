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

#ifndef __INET_NETWORKCONFIGURATOR_H
#define __INET_NETWORKCONFIGURATOR_H

#include <omnetpp.h>
#include "INETDefs.h"
#include "IPAddress.h"

class IInterfaceTable;
class IRoutingTable;


/**
 * Configures IP addresses and routing tables for a network.
 * For more info please see the NED file.
 */
class INET_API NetworkConfigurator : public cSimpleModule
{
  protected:
    struct NodeInfo {
        NodeInfo() {isIPNode=false;isHost=false;isRouter=false;isCore=false;isAggre=false;isEdge=false;isController=false;hostId=0;ift=NULL;rt=NULL;}
        bool isIPNode;
        bool isCore;
        bool isAggre;
        bool isEdge;
        bool isRouter;
        bool isHost;
        bool isController;
        int podId;
        int switchId;
        int hostId;
        IPAddress address;
        IInterfaceTable *ift;
        IRoutingTable *rt;
        IRoutingTable *rt1;
        bool usesDefaultRoute;
    };
    typedef std::vector<NodeInfo> NodeInfoVector;

  protected:
    virtual int numInitStages() const  {return 3;}
    virtual void initialize(int stage);
    virtual void handleMessage(cMessage *msg);

    virtual void extractTopology(cTopology& topo, NodeInfoVector& nodeInfo);
    virtual void assignAddresses(cTopology& topo, NodeInfoVector& nodeInfo);
    //virtual void addPointToPointPeerRoutes(cTopology& topo, NodeInfoVector& nodeInfo);
    virtual void addDefaultRoutes(cTopology& topo, NodeInfoVector& nodeInfo);
    //virtual void setPeersParameter(const char *submodName, cTopology& topo, NodeInfoVector& nodeInfo);
    virtual void fillRoutingTables(cTopology& topo, NodeInfoVector& nodeInfo);

    virtual void setDisplayString(cTopology& topo, NodeInfoVector& nodeInfo);
};

#endif

