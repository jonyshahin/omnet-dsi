//
// Generated file, do not edit! Created by opp_msgc 4.0 from networklayer/rsvp_te/RSVPResvMsg.msg.
//

#ifndef _RSVPRESVMSG_M_H_
#define _RSVPRESVMSG_M_H_

#include <omnetpp.h>

// opp_msgc version check
#define MSGC_VERSION 0x0400
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of opp_msgc: 'make clean' should help.
#endif

// cplusplus {{ ... }} section:

#include "IntServ.h"
#include "RSVPPacket.h"
// end cplusplus



/**
 * Class generated from <tt>networklayer/rsvp_te/RSVPResvMsg.msg</tt> by opp_msgc.
 * <pre>
 * packet RSVPResvMsg extends RSVPPacket
 * {
 *     @customize(true);
 *     RsvpHopObj_t hop;
 *     FlowDescriptorVector flowDescriptor;
 *     int rsvpKind = RESV_MESSAGE;
 * }
 * </pre>
 *
 * RSVPResvMsg_Base is only useful if it gets subclassed, and RSVPResvMsg is derived from it.
 * The minimum code to be written for RSVPResvMsg is the following:
 *
 * <pre>
 * class RSVPResvMsg : public RSVPResvMsg_Base
 * {
 *   public:
 *     RSVPResvMsg(const char *name=NULL, int kind=0) : RSVPResvMsg_Base(name,kind) {}
 *     RSVPResvMsg(const RSVPResvMsg& other) : RSVPResvMsg_Base(other.getName()) {operator=(other);}
 *     RSVPResvMsg& operator=(const RSVPResvMsg& other) {RSVPResvMsg_Base::operator=(other); return *this;}
 *     virtual RSVPResvMsg *dup() const {return new RSVPResvMsg(*this);}
 *     // ADD CODE HERE to redefine and implement pure virtual functions from RSVPResvMsg_Base
 * };
 * </pre>
 *
 * The following should go into a .cc (.cpp) file:
 *
 * <pre>
 * Register_Class(RSVPResvMsg);
 * </pre>
 */
class RSVPResvMsg_Base : public RSVPPacket
{
  protected:
    RsvpHopObj_t hop_var;
    FlowDescriptorVector flowDescriptor_var;
    int rsvpKind_var;

    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const RSVPResvMsg_Base&);
    // make constructors protected to avoid instantiation
    RSVPResvMsg_Base(const char *name=NULL, int kind=0);
    RSVPResvMsg_Base(const RSVPResvMsg_Base& other);
    // make assignment operator protected to force the user override it
    RSVPResvMsg_Base& operator=(const RSVPResvMsg_Base& other);

  public:
    virtual ~RSVPResvMsg_Base();
    virtual RSVPResvMsg_Base *dup() const {throw cRuntimeError("You forgot to manually add a dup() function to class RSVPResvMsg");}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual RsvpHopObj_t& getHop();
    virtual const RsvpHopObj_t& getHop() const {return const_cast<RSVPResvMsg_Base*>(this)->getHop();}
    virtual void setHop(const RsvpHopObj_t& hop_var);
    virtual FlowDescriptorVector& getFlowDescriptor();
    virtual const FlowDescriptorVector& getFlowDescriptor() const {return const_cast<RSVPResvMsg_Base*>(this)->getFlowDescriptor();}
    virtual void setFlowDescriptor(const FlowDescriptorVector& flowDescriptor_var);
    virtual int getRsvpKind() const;
    virtual void setRsvpKind(int rsvpKind_var);
};

/**
 * Class generated from <tt>networklayer/rsvp_te/RSVPResvMsg.msg</tt> by opp_msgc.
 * <pre>
 * packet RSVPResvTear extends RSVPPacket
 * {
 *     @customize(true);
 *     RsvpHopObj_t hop;
 *     FlowDescriptorVector flowDescriptor;
 *     int rsvpKind = RTEAR_MESSAGE;
 * }
 * </pre>
 *
 * RSVPResvTear_Base is only useful if it gets subclassed, and RSVPResvTear is derived from it.
 * The minimum code to be written for RSVPResvTear is the following:
 *
 * <pre>
 * class RSVPResvTear : public RSVPResvTear_Base
 * {
 *   public:
 *     RSVPResvTear(const char *name=NULL, int kind=0) : RSVPResvTear_Base(name,kind) {}
 *     RSVPResvTear(const RSVPResvTear& other) : RSVPResvTear_Base(other.getName()) {operator=(other);}
 *     RSVPResvTear& operator=(const RSVPResvTear& other) {RSVPResvTear_Base::operator=(other); return *this;}
 *     virtual RSVPResvTear *dup() const {return new RSVPResvTear(*this);}
 *     // ADD CODE HERE to redefine and implement pure virtual functions from RSVPResvTear_Base
 * };
 * </pre>
 *
 * The following should go into a .cc (.cpp) file:
 *
 * <pre>
 * Register_Class(RSVPResvTear);
 * </pre>
 */
class RSVPResvTear_Base : public RSVPPacket
{
  protected:
    RsvpHopObj_t hop_var;
    FlowDescriptorVector flowDescriptor_var;
    int rsvpKind_var;

    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const RSVPResvTear_Base&);
    // make constructors protected to avoid instantiation
    RSVPResvTear_Base(const char *name=NULL, int kind=0);
    RSVPResvTear_Base(const RSVPResvTear_Base& other);
    // make assignment operator protected to force the user override it
    RSVPResvTear_Base& operator=(const RSVPResvTear_Base& other);

  public:
    virtual ~RSVPResvTear_Base();
    virtual RSVPResvTear_Base *dup() const {throw cRuntimeError("You forgot to manually add a dup() function to class RSVPResvTear");}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual RsvpHopObj_t& getHop();
    virtual const RsvpHopObj_t& getHop() const {return const_cast<RSVPResvTear_Base*>(this)->getHop();}
    virtual void setHop(const RsvpHopObj_t& hop_var);
    virtual FlowDescriptorVector& getFlowDescriptor();
    virtual const FlowDescriptorVector& getFlowDescriptor() const {return const_cast<RSVPResvTear_Base*>(this)->getFlowDescriptor();}
    virtual void setFlowDescriptor(const FlowDescriptorVector& flowDescriptor_var);
    virtual int getRsvpKind() const;
    virtual void setRsvpKind(int rsvpKind_var);
};

/**
 * Class generated from <tt>networklayer/rsvp_te/RSVPResvMsg.msg</tt> by opp_msgc.
 * <pre>
 * packet RSVPResvError extends RSVPPacket
 * {
 *     @customize(true);
 *     RsvpHopObj_t hop;
 *     IPAddress errorNode;
 *     int errorCode;
 *     int rsvpKind = RERROR_MESSAGE;
 * }
 * </pre>
 *
 * RSVPResvError_Base is only useful if it gets subclassed, and RSVPResvError is derived from it.
 * The minimum code to be written for RSVPResvError is the following:
 *
 * <pre>
 * class RSVPResvError : public RSVPResvError_Base
 * {
 *   public:
 *     RSVPResvError(const char *name=NULL, int kind=0) : RSVPResvError_Base(name,kind) {}
 *     RSVPResvError(const RSVPResvError& other) : RSVPResvError_Base(other.getName()) {operator=(other);}
 *     RSVPResvError& operator=(const RSVPResvError& other) {RSVPResvError_Base::operator=(other); return *this;}
 *     virtual RSVPResvError *dup() const {return new RSVPResvError(*this);}
 *     // ADD CODE HERE to redefine and implement pure virtual functions from RSVPResvError_Base
 * };
 * </pre>
 *
 * The following should go into a .cc (.cpp) file:
 *
 * <pre>
 * Register_Class(RSVPResvError);
 * </pre>
 */
class RSVPResvError_Base : public RSVPPacket
{
  protected:
    RsvpHopObj_t hop_var;
    IPAddress errorNode_var;
    int errorCode_var;
    int rsvpKind_var;

    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const RSVPResvError_Base&);
    // make constructors protected to avoid instantiation
    RSVPResvError_Base(const char *name=NULL, int kind=0);
    RSVPResvError_Base(const RSVPResvError_Base& other);
    // make assignment operator protected to force the user override it
    RSVPResvError_Base& operator=(const RSVPResvError_Base& other);

  public:
    virtual ~RSVPResvError_Base();
    virtual RSVPResvError_Base *dup() const {throw cRuntimeError("You forgot to manually add a dup() function to class RSVPResvError");}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual RsvpHopObj_t& getHop();
    virtual const RsvpHopObj_t& getHop() const {return const_cast<RSVPResvError_Base*>(this)->getHop();}
    virtual void setHop(const RsvpHopObj_t& hop_var);
    virtual IPAddress& getErrorNode();
    virtual const IPAddress& getErrorNode() const {return const_cast<RSVPResvError_Base*>(this)->getErrorNode();}
    virtual void setErrorNode(const IPAddress& errorNode_var);
    virtual int getErrorCode() const;
    virtual void setErrorCode(int errorCode_var);
    virtual int getRsvpKind() const;
    virtual void setRsvpKind(int rsvpKind_var);
};


#endif // _RSVPRESVMSG_M_H_
