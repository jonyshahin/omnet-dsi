//
// Generated file, do not edit! Created by opp_msgc 4.0 from linklayer/ieee80211/mac/Ieee80211Frame.msg.
//

#ifndef _IEEE80211FRAME_M_H_
#define _IEEE80211FRAME_M_H_

#include <omnetpp.h>

// opp_msgc version check
#define MSGC_VERSION 0x0400
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of opp_msgc: 'make clean' should help.
#endif

// cplusplus {{ ... }} section:


#include "MACAddress.h"
// end cplusplus



/**
 * Enum generated from <tt>linklayer/ieee80211/mac/Ieee80211Frame.msg</tt> by opp_msgc.
 * <pre>
 * enum Ieee80211FrameType
 * {
 * 
 *     
 *     ST_ASSOCIATIONREQUEST = 0x00;
 *     ST_ASSOCIATIONRESPONSE = 0x01;
 *     ST_REASSOCIATIONREQUEST = 0x02;
 *     ST_REASSOCIATIONRESPONSE = 0x03;
 *     ST_PROBEREQUEST = 0x04;
 *     ST_PROBERESPONSE = 0x05;
 *     ST_BEACON = 0x08;
 *     ST_ATIM = 0x09;
 *     ST_DISASSOCIATION = 0x0a;
 *     ST_AUTHENTICATION = 0x0b;
 *     ST_DEAUTHENTICATION = 0x0c;
 * 
 *     
 *     ST_PSPOLL = 0x1a;
 *     ST_RTS = 0x1b;
 *     ST_CTS = 0x1c;
 *     ST_ACK = 0x1d;
 * 
 *     
 *     ST_DATA = 0x20;
 * }
 * </pre>
 */
enum Ieee80211FrameType {
    ST_ASSOCIATIONREQUEST = 0x00,
    ST_ASSOCIATIONRESPONSE = 0x01,
    ST_REASSOCIATIONREQUEST = 0x02,
    ST_REASSOCIATIONRESPONSE = 0x03,
    ST_PROBEREQUEST = 0x04,
    ST_PROBERESPONSE = 0x05,
    ST_BEACON = 0x08,
    ST_ATIM = 0x09,
    ST_DISASSOCIATION = 0x0a,
    ST_AUTHENTICATION = 0x0b,
    ST_DEAUTHENTICATION = 0x0c,
    ST_PSPOLL = 0x1a,
    ST_RTS = 0x1b,
    ST_CTS = 0x1c,
    ST_ACK = 0x1d,
    ST_DATA = 0x20
};

/**
 * Class generated from <tt>linklayer/ieee80211/mac/Ieee80211Frame.msg</tt> by opp_msgc.
 * <pre>
 * packet Ieee80211Frame
 * {
 *     byteLength = 14;
 *     short type enum(Ieee80211FrameType); 
 *     bool toDS;
 *     bool fromDS;
 *     bool retry;
 *     bool moreFragments;
 *     simtime_t duration = -1; 
 *     short AID = -1;          
 *     MACAddress receiverAddress; 
 * }
 * </pre>
 */
class Ieee80211Frame : public cPacket
{
  protected:
    short type_var;
    bool toDS_var;
    bool fromDS_var;
    bool retry_var;
    bool moreFragments_var;
    simtime_t duration_var;
    short AID_var;
    MACAddress receiverAddress_var;

    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const Ieee80211Frame&);

  public:
    Ieee80211Frame(const char *name=NULL, int kind=0);
    Ieee80211Frame(const Ieee80211Frame& other);
    virtual ~Ieee80211Frame();
    Ieee80211Frame& operator=(const Ieee80211Frame& other);
    virtual Ieee80211Frame *dup() const {return new Ieee80211Frame(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual short getType() const;
    virtual void setType(short type_var);
    virtual bool getToDS() const;
    virtual void setToDS(bool toDS_var);
    virtual bool getFromDS() const;
    virtual void setFromDS(bool fromDS_var);
    virtual bool getRetry() const;
    virtual void setRetry(bool retry_var);
    virtual bool getMoreFragments() const;
    virtual void setMoreFragments(bool moreFragments_var);
    virtual simtime_t getDuration() const;
    virtual void setDuration(simtime_t duration_var);
    virtual short getAID() const;
    virtual void setAID(short AID_var);
    virtual MACAddress& getReceiverAddress();
    virtual const MACAddress& getReceiverAddress() const {return const_cast<Ieee80211Frame*>(this)->getReceiverAddress();}
    virtual void setReceiverAddress(const MACAddress& receiverAddress_var);
};

inline void doPacking(cCommBuffer *b, Ieee80211Frame& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, Ieee80211Frame& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>linklayer/ieee80211/mac/Ieee80211Frame.msg</tt> by opp_msgc.
 * <pre>
 * packet Ieee80211OneAddressFrame extends Ieee80211Frame
 * {
 * }
 * </pre>
 */
class Ieee80211OneAddressFrame : public Ieee80211Frame
{
  protected:

    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const Ieee80211OneAddressFrame&);

  public:
    Ieee80211OneAddressFrame(const char *name=NULL, int kind=0);
    Ieee80211OneAddressFrame(const Ieee80211OneAddressFrame& other);
    virtual ~Ieee80211OneAddressFrame();
    Ieee80211OneAddressFrame& operator=(const Ieee80211OneAddressFrame& other);
    virtual Ieee80211OneAddressFrame *dup() const {return new Ieee80211OneAddressFrame(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
};

inline void doPacking(cCommBuffer *b, Ieee80211OneAddressFrame& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, Ieee80211OneAddressFrame& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>linklayer/ieee80211/mac/Ieee80211Frame.msg</tt> by opp_msgc.
 * <pre>
 * packet Ieee80211ACKFrame extends Ieee80211OneAddressFrame
 * {
 *     type = ST_ACK;
 * }
 * </pre>
 */
class Ieee80211ACKFrame : public Ieee80211OneAddressFrame
{
  protected:

    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const Ieee80211ACKFrame&);

  public:
    Ieee80211ACKFrame(const char *name=NULL, int kind=0);
    Ieee80211ACKFrame(const Ieee80211ACKFrame& other);
    virtual ~Ieee80211ACKFrame();
    Ieee80211ACKFrame& operator=(const Ieee80211ACKFrame& other);
    virtual Ieee80211ACKFrame *dup() const {return new Ieee80211ACKFrame(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
};

inline void doPacking(cCommBuffer *b, Ieee80211ACKFrame& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, Ieee80211ACKFrame& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>linklayer/ieee80211/mac/Ieee80211Frame.msg</tt> by opp_msgc.
 * <pre>
 * packet Ieee80211TwoAddressFrame extends Ieee80211OneAddressFrame
 * {
 *     byteLength = 20;
 *     MACAddress transmitterAddress; 
 * }
 * </pre>
 */
class Ieee80211TwoAddressFrame : public Ieee80211OneAddressFrame
{
  protected:
    MACAddress transmitterAddress_var;

    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const Ieee80211TwoAddressFrame&);

  public:
    Ieee80211TwoAddressFrame(const char *name=NULL, int kind=0);
    Ieee80211TwoAddressFrame(const Ieee80211TwoAddressFrame& other);
    virtual ~Ieee80211TwoAddressFrame();
    Ieee80211TwoAddressFrame& operator=(const Ieee80211TwoAddressFrame& other);
    virtual Ieee80211TwoAddressFrame *dup() const {return new Ieee80211TwoAddressFrame(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual MACAddress& getTransmitterAddress();
    virtual const MACAddress& getTransmitterAddress() const {return const_cast<Ieee80211TwoAddressFrame*>(this)->getTransmitterAddress();}
    virtual void setTransmitterAddress(const MACAddress& transmitterAddress_var);
};

inline void doPacking(cCommBuffer *b, Ieee80211TwoAddressFrame& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, Ieee80211TwoAddressFrame& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>linklayer/ieee80211/mac/Ieee80211Frame.msg</tt> by opp_msgc.
 * <pre>
 * packet Ieee80211RTSFrame extends Ieee80211TwoAddressFrame
 * {
 *     type = ST_RTS;
 * }
 * </pre>
 */
class Ieee80211RTSFrame : public Ieee80211TwoAddressFrame
{
  protected:

    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const Ieee80211RTSFrame&);

  public:
    Ieee80211RTSFrame(const char *name=NULL, int kind=0);
    Ieee80211RTSFrame(const Ieee80211RTSFrame& other);
    virtual ~Ieee80211RTSFrame();
    Ieee80211RTSFrame& operator=(const Ieee80211RTSFrame& other);
    virtual Ieee80211RTSFrame *dup() const {return new Ieee80211RTSFrame(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
};

inline void doPacking(cCommBuffer *b, Ieee80211RTSFrame& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, Ieee80211RTSFrame& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>linklayer/ieee80211/mac/Ieee80211Frame.msg</tt> by opp_msgc.
 * <pre>
 * packet Ieee80211CTSFrame extends Ieee80211OneAddressFrame
 * {
 *     type = ST_CTS;
 * }
 * </pre>
 */
class Ieee80211CTSFrame : public Ieee80211OneAddressFrame
{
  protected:

    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const Ieee80211CTSFrame&);

  public:
    Ieee80211CTSFrame(const char *name=NULL, int kind=0);
    Ieee80211CTSFrame(const Ieee80211CTSFrame& other);
    virtual ~Ieee80211CTSFrame();
    Ieee80211CTSFrame& operator=(const Ieee80211CTSFrame& other);
    virtual Ieee80211CTSFrame *dup() const {return new Ieee80211CTSFrame(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
};

inline void doPacking(cCommBuffer *b, Ieee80211CTSFrame& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, Ieee80211CTSFrame& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>linklayer/ieee80211/mac/Ieee80211Frame.msg</tt> by opp_msgc.
 * <pre>
 * packet Ieee80211DataOrMgmtFrame extends Ieee80211TwoAddressFrame
 * {
 *     byteLength = 28;
 *     MACAddress address3;
 *     short fragmentNumber;
 *     short sequenceNumber;
 * }
 * </pre>
 */
class Ieee80211DataOrMgmtFrame : public Ieee80211TwoAddressFrame
{
  protected:
    MACAddress address3_var;
    short fragmentNumber_var;
    short sequenceNumber_var;

    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const Ieee80211DataOrMgmtFrame&);

  public:
    Ieee80211DataOrMgmtFrame(const char *name=NULL, int kind=0);
    Ieee80211DataOrMgmtFrame(const Ieee80211DataOrMgmtFrame& other);
    virtual ~Ieee80211DataOrMgmtFrame();
    Ieee80211DataOrMgmtFrame& operator=(const Ieee80211DataOrMgmtFrame& other);
    virtual Ieee80211DataOrMgmtFrame *dup() const {return new Ieee80211DataOrMgmtFrame(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual MACAddress& getAddress3();
    virtual const MACAddress& getAddress3() const {return const_cast<Ieee80211DataOrMgmtFrame*>(this)->getAddress3();}
    virtual void setAddress3(const MACAddress& address3_var);
    virtual short getFragmentNumber() const;
    virtual void setFragmentNumber(short fragmentNumber_var);
    virtual short getSequenceNumber() const;
    virtual void setSequenceNumber(short sequenceNumber_var);
};

inline void doPacking(cCommBuffer *b, Ieee80211DataOrMgmtFrame& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, Ieee80211DataOrMgmtFrame& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>linklayer/ieee80211/mac/Ieee80211Frame.msg</tt> by opp_msgc.
 * <pre>
 * packet Ieee80211DataFrame extends Ieee80211DataOrMgmtFrame
 * {
 *     type = ST_DATA;
 *     byteLength = 34;
 *     MACAddress address4;
 * }
 * </pre>
 */
class Ieee80211DataFrame : public Ieee80211DataOrMgmtFrame
{
  protected:
    MACAddress address4_var;

    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const Ieee80211DataFrame&);

  public:
    Ieee80211DataFrame(const char *name=NULL, int kind=0);
    Ieee80211DataFrame(const Ieee80211DataFrame& other);
    virtual ~Ieee80211DataFrame();
    Ieee80211DataFrame& operator=(const Ieee80211DataFrame& other);
    virtual Ieee80211DataFrame *dup() const {return new Ieee80211DataFrame(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual MACAddress& getAddress4();
    virtual const MACAddress& getAddress4() const {return const_cast<Ieee80211DataFrame*>(this)->getAddress4();}
    virtual void setAddress4(const MACAddress& address4_var);
};

inline void doPacking(cCommBuffer *b, Ieee80211DataFrame& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, Ieee80211DataFrame& obj) {obj.parsimUnpack(b);}

/**
 * Class generated from <tt>linklayer/ieee80211/mac/Ieee80211Frame.msg</tt> by opp_msgc.
 * <pre>
 * packet Ieee80211ManagementFrame extends Ieee80211DataOrMgmtFrame
 * {
 * }
 * </pre>
 */
class Ieee80211ManagementFrame : public Ieee80211DataOrMgmtFrame
{
  protected:

    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const Ieee80211ManagementFrame&);

  public:
    Ieee80211ManagementFrame(const char *name=NULL, int kind=0);
    Ieee80211ManagementFrame(const Ieee80211ManagementFrame& other);
    virtual ~Ieee80211ManagementFrame();
    Ieee80211ManagementFrame& operator=(const Ieee80211ManagementFrame& other);
    virtual Ieee80211ManagementFrame *dup() const {return new Ieee80211ManagementFrame(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
};

inline void doPacking(cCommBuffer *b, Ieee80211ManagementFrame& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, Ieee80211ManagementFrame& obj) {obj.parsimUnpack(b);}


#endif // _IEEE80211FRAME_M_H_
