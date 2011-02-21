//
// Generated file, do not edit! Created by opp_msgc 4.0 from networklayer/contract/IPControlInfo.msg.
//

#ifndef _IPCONTROLINFO_M_H_
#define _IPCONTROLINFO_M_H_

#include <omnetpp.h>

// opp_msgc version check
#define MSGC_VERSION 0x0400
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of opp_msgc: 'make clean' should help.
#endif

// cplusplus {{ ... }} section:

#include "IPAddress.h"
#include "IPProtocolId_m.h"
// end cplusplus



/**
 * Class generated from <tt>networklayer/contract/IPControlInfo.msg</tt> by opp_msgc.
 * <pre>
 * class IPControlInfo
 * {
 *     @customize(true);
 *     IPAddress destAddr;   
 *     IPAddress srcAddr;    
 *     int destPort;
 *     int srcPort;
 *     int flowId;
 *     int packetId;
 *     int link_status;
 *     float load;
 *     int num_packets_for_this_path;
 *     int interfaceId = -1; 
 *                               
 *     short protocol enum(IPProtocolId);  
 *     unsigned char diffServCodePoint;  
 *     short timeToLive;     
 *     bool dontFragment;    
 * }
 * </pre>
 *
 * IPControlInfo_Base is only useful if it gets subclassed, and IPControlInfo is derived from it.
 * The minimum code to be written for IPControlInfo is the following:
 *
 * <pre>
 * class IPControlInfo : public IPControlInfo_Base
 * {
 *   public:
 *     IPControlInfo() : IPControlInfo_Base() {}
 *     IPControlInfo(const IPControlInfo& other) : IPControlInfo_Base() {operator=(other);}
 *     IPControlInfo& operator=(const IPControlInfo& other) {IPControlInfo_Base::operator=(other); return *this;}
 *     virtual IPControlInfo *dup() const {return new IPControlInfo(*this);}
 *     // ADD CODE HERE to redefine and implement pure virtual functions from IPControlInfo_Base
 * };
 * </pre>
 *
 * The following should go into a .cc (.cpp) file:
 *
 * <pre>
 * Register_Class(IPControlInfo);
 * </pre>
 */
class IPControlInfo_Base : public cObject
{
  protected:
    IPAddress destAddr_var;
    IPAddress srcAddr_var;
    int destPort_var;
    int srcPort_var;
    int flowId_var;
    int packetId_var;
    int link_status_var;
    float load_var;
    int num_packets_for_this_path_var;
    int interfaceId_var;
    short protocol_var;
    unsigned char diffServCodePoint_var;
    short timeToLive_var;
    bool dontFragment_var;

    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const IPControlInfo_Base&);
    // make constructors protected to avoid instantiation
    IPControlInfo_Base();
    IPControlInfo_Base(const IPControlInfo_Base& other);
    // make assignment operator protected to force the user override it
    IPControlInfo_Base& operator=(const IPControlInfo_Base& other);

  public:
    virtual ~IPControlInfo_Base();
    virtual IPControlInfo_Base *dup() const {throw cRuntimeError("You forgot to manually add a dup() function to class IPControlInfo");}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual IPAddress& getDestAddr();
    virtual const IPAddress& getDestAddr() const {return const_cast<IPControlInfo_Base*>(this)->getDestAddr();}
    virtual void setDestAddr(const IPAddress& destAddr_var);
    virtual IPAddress& getSrcAddr();
    virtual const IPAddress& getSrcAddr() const {return const_cast<IPControlInfo_Base*>(this)->getSrcAddr();}
    virtual void setSrcAddr(const IPAddress& srcAddr_var);
    virtual int getDestPort() const;
    virtual void setDestPort(int destPort_var);
    virtual int getSrcPort() const;
    virtual void setSrcPort(int srcPort_var);
    virtual int getFlowId() const;
    virtual void setFlowId(int flowId_var);
    virtual int getPacketId() const;
    virtual void setPacketId(int packetId_var);
    virtual int getLink_status() const;
    virtual void setLink_status(int link_status_var);
    virtual float getLoad() const;
    virtual void setLoad(float load_var);
    virtual int getNum_packets_for_this_path() const;
    virtual void setNum_packets_for_this_path(int num_packets_for_this_path_var);
    virtual int getInterfaceId() const;
    virtual void setInterfaceId(int interfaceId_var);
    virtual short getProtocol() const;
    virtual void setProtocol(short protocol_var);
    virtual unsigned char getDiffServCodePoint() const;
    virtual void setDiffServCodePoint(unsigned char diffServCodePoint_var);
    virtual short getTimeToLive() const;
    virtual void setTimeToLive(short timeToLive_var);
    virtual bool getDontFragment() const;
    virtual void setDontFragment(bool dontFragment_var);
};

/**
 * Class generated from <tt>networklayer/contract/IPControlInfo.msg</tt> by opp_msgc.
 * <pre>
 * class IPRoutingDecision
 * {
 *     int interfaceId = -1; 
 *     IPAddress nextHopAddr;
 * }
 * </pre>
 */
class IPRoutingDecision : public cObject
{
  protected:
    int interfaceId_var;
    IPAddress nextHopAddr_var;

    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const IPRoutingDecision&);

  public:
    IPRoutingDecision();
    IPRoutingDecision(const IPRoutingDecision& other);
    virtual ~IPRoutingDecision();
    IPRoutingDecision& operator=(const IPRoutingDecision& other);
    virtual IPRoutingDecision *dup() const {return new IPRoutingDecision(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual int getInterfaceId() const;
    virtual void setInterfaceId(int interfaceId_var);
    virtual IPAddress& getNextHopAddr();
    virtual const IPAddress& getNextHopAddr() const {return const_cast<IPRoutingDecision*>(this)->getNextHopAddr();}
    virtual void setNextHopAddr(const IPAddress& nextHopAddr_var);
};

inline void doPacking(cCommBuffer *b, IPRoutingDecision& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, IPRoutingDecision& obj) {obj.parsimUnpack(b);}


#endif // _IPCONTROLINFO_M_H_
