//
// Generated file, do not edit! Created by opp_msgc 4.0 from networklayer/ted/LinkStatePacket.msg.
//

#ifndef _LINKSTATEPACKET_M_H_
#define _LINKSTATEPACKET_M_H_

#include <omnetpp.h>

// opp_msgc version check
#define MSGC_VERSION 0x0400
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of opp_msgc: 'make clean' should help.
#endif

// cplusplus {{ ... }} section:

#include "TED_m.h"
// end cplusplus



/**
 * Class generated from <tt>networklayer/ted/LinkStatePacket.msg</tt> by opp_msgc.
 * <pre>
 * packet LinkStateMsg
 * {
 *     TELinkStateInfo linkInfo[];
 * 
 *     bool request = false; 
 *         
 * 
 *     int command = LINK_STATE_MESSAGE; 
 * }
 * </pre>
 */
class LinkStateMsg : public cPacket
{
  protected:
    TELinkStateInfo *linkInfo_var; // array ptr
    unsigned int linkInfo_arraysize;
    bool request_var;
    int command_var;

    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const LinkStateMsg&);

  public:
    LinkStateMsg(const char *name=NULL, int kind=0);
    LinkStateMsg(const LinkStateMsg& other);
    virtual ~LinkStateMsg();
    LinkStateMsg& operator=(const LinkStateMsg& other);
    virtual LinkStateMsg *dup() const {return new LinkStateMsg(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual void setLinkInfoArraySize(unsigned int size);
    virtual unsigned int getLinkInfoArraySize() const;
    virtual TELinkStateInfo& getLinkInfo(unsigned int k);
    virtual const TELinkStateInfo& getLinkInfo(unsigned int k) const {return const_cast<LinkStateMsg*>(this)->getLinkInfo(k);}
    virtual void setLinkInfo(unsigned int k, const TELinkStateInfo& linkInfo_var);
    virtual bool getRequest() const;
    virtual void setRequest(bool request_var);
    virtual int getCommand() const;
    virtual void setCommand(int command_var);
};

inline void doPacking(cCommBuffer *b, LinkStateMsg& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, LinkStateMsg& obj) {obj.parsimUnpack(b);}


#endif // _LINKSTATEPACKET_M_H_
