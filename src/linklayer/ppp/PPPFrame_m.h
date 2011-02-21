//
// Generated file, do not edit! Created by opp_msgc 4.0 from linklayer/ppp/PPPFrame.msg.
//

#ifndef _PPPFRAME_M_H_
#define _PPPFRAME_M_H_

#include <omnetpp.h>

// opp_msgc version check
#define MSGC_VERSION 0x0400
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of opp_msgc: 'make clean' should help.
#endif

// cplusplus {{ ... }} section:

#include "INETDefs.h"
const int PPP_OVERHEAD_BYTES = 7;
// end cplusplus



/**
 * Class generated from <tt>linklayer/ppp/PPPFrame.msg</tt> by opp_msgc.
 * <pre>
 * packet PPPFrame
 * {
 * }
 * </pre>
 */
class PPPFrame : public cPacket
{
  protected:

    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const PPPFrame&);

  public:
    PPPFrame(const char *name=NULL, int kind=0);
    PPPFrame(const PPPFrame& other);
    virtual ~PPPFrame();
    PPPFrame& operator=(const PPPFrame& other);
    virtual PPPFrame *dup() const {return new PPPFrame(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
};

inline void doPacking(cCommBuffer *b, PPPFrame& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, PPPFrame& obj) {obj.parsimUnpack(b);}


#endif // _PPPFRAME_M_H_
