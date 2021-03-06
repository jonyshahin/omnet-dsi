//
// Generated file, do not edit! Created by opp_msgc 4.0 from applications/pingapp/PingPayload.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#include <iostream>
#include <sstream>
#include "PingPayload_m.h"

// Template rule which fires if a struct or class doesn't have operator<<
template<typename T>
std::ostream& operator<<(std::ostream& out,const T&) {return out;}

// Another default rule (prevents compiler from choosing base class' doPacking())
template<typename T>
void doPacking(cCommBuffer *, T& t) {
    throw cRuntimeError("Parsim error: no doPacking() function for type %s or its base class (check .msg and _m.cc/h files!)",opp_typename(typeid(t)));
}

template<typename T>
void doUnpacking(cCommBuffer *, T& t) {
    throw cRuntimeError("Parsim error: no doUnpacking() function for type %s or its base class (check .msg and _m.cc/h files!)",opp_typename(typeid(t)));
}




Register_Class(PingPayload);

PingPayload::PingPayload(const char *name, int kind) : cPacket(name,kind)
{
    this->originatorId_var = 0;
    this->seqNo_var = 0;
    data_arraysize = 0;
    this->data_var = 0;
}

PingPayload::PingPayload(const PingPayload& other) : cPacket()
{
    setName(other.getName());
    data_arraysize = 0;
    this->data_var = 0;
    operator=(other);
}

PingPayload::~PingPayload()
{
    delete [] data_var;
}

PingPayload& PingPayload::operator=(const PingPayload& other)
{
    if (this==&other) return *this;
    cPacket::operator=(other);
    this->originatorId_var = other.originatorId_var;
    this->seqNo_var = other.seqNo_var;
    delete [] this->data_var;
    this->data_var = (other.data_arraysize==0) ? NULL : new unsigned char[other.data_arraysize];
    data_arraysize = other.data_arraysize;
    for (unsigned int i=0; i<data_arraysize; i++)
        this->data_var[i] = other.data_var[i];
    return *this;
}

void PingPayload::parsimPack(cCommBuffer *b)
{
    cPacket::parsimPack(b);
    doPacking(b,this->originatorId_var);
    doPacking(b,this->seqNo_var);
    b->pack(data_arraysize);
    doPacking(b,this->data_var,data_arraysize);
}

void PingPayload::parsimUnpack(cCommBuffer *b)
{
    cPacket::parsimUnpack(b);
    doUnpacking(b,this->originatorId_var);
    doUnpacking(b,this->seqNo_var);
    delete [] this->data_var;
    b->unpack(data_arraysize);
    if (data_arraysize==0) {
        this->data_var = 0;
    } else {
        this->data_var = new unsigned char[data_arraysize];
        doUnpacking(b,this->data_var,data_arraysize);
    }
}

long PingPayload::getOriginatorId() const
{
    return originatorId_var;
}

void PingPayload::setOriginatorId(long originatorId_var)
{
    this->originatorId_var = originatorId_var;
}

long PingPayload::getSeqNo() const
{
    return seqNo_var;
}

void PingPayload::setSeqNo(long seqNo_var)
{
    this->seqNo_var = seqNo_var;
}

void PingPayload::setDataArraySize(unsigned int size)
{
    unsigned char *data_var2 = (size==0) ? NULL : new unsigned char[size];
    unsigned int sz = data_arraysize < size ? data_arraysize : size;
    for (unsigned int i=0; i<sz; i++)
        data_var2[i] = this->data_var[i];
    for (unsigned int i=sz; i<size; i++)
        data_var2[i] = 0;
    data_arraysize = size;
    delete [] this->data_var;
    this->data_var = data_var2;
}

unsigned int PingPayload::getDataArraySize() const
{
    return data_arraysize;
}

unsigned char PingPayload::getData(unsigned int k) const
{
    if (k>=data_arraysize) throw cRuntimeError("Array of size %d indexed by %d", data_arraysize, k);
    return data_var[k];
}

void PingPayload::setData(unsigned int k, unsigned char data_var)
{
    if (k>=data_arraysize) throw cRuntimeError("Array of size %d indexed by %d", data_arraysize, k);
    this->data_var[k]=data_var;
}

class PingPayloadDescriptor : public cClassDescriptor
{
  public:
    PingPayloadDescriptor();
    virtual ~PingPayloadDescriptor();

    virtual bool doesSupport(cObject *obj) const;
    virtual const char *getProperty(const char *propertyname) const;
    virtual int getFieldCount(void *object) const;
    virtual const char *getFieldName(void *object, int field) const;
    virtual unsigned int getFieldTypeFlags(void *object, int field) const;
    virtual const char *getFieldTypeString(void *object, int field) const;
    virtual const char *getFieldProperty(void *object, int field, const char *propertyname) const;
    virtual int getArraySize(void *object, int field) const;

    virtual bool getFieldAsString(void *object, int field, int i, char *resultbuf, int bufsize) const;
    virtual bool setFieldAsString(void *object, int field, int i, const char *value) const;

    virtual const char *getFieldStructName(void *object, int field) const;
    virtual void *getFieldStructPointer(void *object, int field, int i) const;
};

Register_ClassDescriptor(PingPayloadDescriptor);

PingPayloadDescriptor::PingPayloadDescriptor() : cClassDescriptor("PingPayload", "cPacket")
{
}

PingPayloadDescriptor::~PingPayloadDescriptor()
{
}

bool PingPayloadDescriptor::doesSupport(cObject *obj) const
{
    return dynamic_cast<PingPayload *>(obj)!=NULL;
}

const char *PingPayloadDescriptor::getProperty(const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : NULL;
}

int PingPayloadDescriptor::getFieldCount(void *object) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 3+basedesc->getFieldCount(object) : 3;
}

unsigned int PingPayloadDescriptor::getFieldTypeFlags(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeFlags(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return FD_ISEDITABLE;
        case 1: return FD_ISEDITABLE;
        case 2: return FD_ISARRAY | FD_ISEDITABLE;
        default: return 0;
    }
}

const char *PingPayloadDescriptor::getFieldName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return "originatorId";
        case 1: return "seqNo";
        case 2: return "data";
        default: return NULL;
    }
}

const char *PingPayloadDescriptor::getFieldTypeString(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeString(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        case 0: return "long";
        case 1: return "long";
        case 2: return "unsigned char";
        default: return NULL;
    }
}

const char *PingPayloadDescriptor::getFieldProperty(void *object, int field, const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldProperty(object, field, propertyname);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        default: return NULL;
    }
}

int PingPayloadDescriptor::getArraySize(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getArraySize(object, field);
        field -= basedesc->getFieldCount(object);
    }
    PingPayload *pp = (PingPayload *)object; (void)pp;
    switch (field) {
        case 2: return pp->getDataArraySize();
        default: return 0;
    }
}

bool PingPayloadDescriptor::getFieldAsString(void *object, int field, int i, char *resultbuf, int bufsize) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldAsString(object,field,i,resultbuf,bufsize);
        field -= basedesc->getFieldCount(object);
    }
    PingPayload *pp = (PingPayload *)object; (void)pp;
    switch (field) {
        case 0: long2string(pp->getOriginatorId(),resultbuf,bufsize); return true;
        case 1: long2string(pp->getSeqNo(),resultbuf,bufsize); return true;
        case 2: ulong2string(pp->getData(i),resultbuf,bufsize); return true;
        default: return false;
    }
}

bool PingPayloadDescriptor::setFieldAsString(void *object, int field, int i, const char *value) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->setFieldAsString(object,field,i,value);
        field -= basedesc->getFieldCount(object);
    }
    PingPayload *pp = (PingPayload *)object; (void)pp;
    switch (field) {
        case 0: pp->setOriginatorId(string2long(value)); return true;
        case 1: pp->setSeqNo(string2long(value)); return true;
        case 2: pp->setData(i,string2ulong(value)); return true;
        default: return false;
    }
}

const char *PingPayloadDescriptor::getFieldStructName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        default: return NULL;
    }
}

void *PingPayloadDescriptor::getFieldStructPointer(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructPointer(object, field, i);
        field -= basedesc->getFieldCount(object);
    }
    PingPayload *pp = (PingPayload *)object; (void)pp;
    switch (field) {
        default: return NULL;
    }
}


