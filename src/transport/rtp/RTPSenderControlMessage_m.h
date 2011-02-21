//
// Generated file, do not edit! Created by opp_msgc 4.0 from transport/rtp/RTPSenderControlMessage.msg.
//

#ifndef _RTPSENDERCONTROLMESSAGE_M_H_
#define _RTPSENDERCONTROLMESSAGE_M_H_

#include <omnetpp.h>

// opp_msgc version check
#define MSGC_VERSION 0x0400
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of opp_msgc: 'make clean' should help.
#endif

// cplusplus {{ ... }} section:

#include "INETDefs.h"
// end cplusplus



/**
 * Class generated from <tt>transport/rtp/RTPSenderControlMessage.msg</tt> by opp_msgc.
 * <pre>
 * packet RTPSenderControlMessage
 * {
 *     string command;
 *     float commandParameter1;
 *     float commandParameter2;
 * };
 * </pre>
 */
class RTPSenderControlMessage : public cPacket
{
  protected:
    opp_string command_var;
    float commandParameter1_var;
    float commandParameter2_var;

    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const RTPSenderControlMessage&);

  public:
    RTPSenderControlMessage(const char *name=NULL, int kind=0);
    RTPSenderControlMessage(const RTPSenderControlMessage& other);
    virtual ~RTPSenderControlMessage();
    RTPSenderControlMessage& operator=(const RTPSenderControlMessage& other);
    virtual RTPSenderControlMessage *dup() const {return new RTPSenderControlMessage(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual const char * getCommand() const;
    virtual void setCommand(const char * command_var);
    virtual float getCommandParameter1() const;
    virtual void setCommandParameter1(float commandParameter1_var);
    virtual float getCommandParameter2() const;
    virtual void setCommandParameter2(float commandParameter2_var);
};

inline void doPacking(cCommBuffer *b, RTPSenderControlMessage& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, RTPSenderControlMessage& obj) {obj.parsimUnpack(b);}


#endif // _RTPSENDERCONTROLMESSAGE_M_H_
