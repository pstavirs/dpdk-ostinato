/*
Copyright (C) 2010 Srivats P.

This file is part of "Ostinato"

This is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include <qendian.h>

#include "abstractprotocol.h" 
#include "streambase.h"
#include "protocollistiterator.h"

/*!
  \class AbstractProtocol

  AbstractProtocol is the base abstract class which provides the interface
  for all protocols.

  All protocols supported by Ostinato are derived from AbstractProtocol. Apart
  from defining the interface for a protocol, it also provides sensible default
  implementations for methods so that the subclasses need not re-implement. It
  also provides convenience functions for subclasses to use such as methods to
  retrieve payload size, checksum etc.
  
  A subclass typically needs to reimplement the following methods -
  - name()
  - shortName()
  - createInstance()
  - protocolNumber()
  - protoDataCopyInto() [pure virtual]
  - protoDataCopyFrom() [pure virtual]
  - fieldCount()
  - fieldFlags()
  - fieldData()
  - setFieldData()
  - configWidget() [pure virtual]
  - loadConfigWidget() [pure virtual]
  - storeConfigWidget() [pure virtual]

  Depending on certain conditions, subclasses may need to reimplement the
  following additional methods -
  - protocolIdType()
  - protocolId()
  - protocolFrameSize()
  - isProtocolFrameValueVariable()
  - isProtocolFrameSizeVariable()

  See the description of the methods for more information.

  Most of the above methods just need some standard boilerplate code - 
  the SampleProtocol implementation includes the boilerplate
*/

/*!
  Constructs an abstract protocol for the given stream and parent

  parent is typically NULL except for protocols which are part of a 
  ComboProtocol
*/
AbstractProtocol::AbstractProtocol(StreamBase *stream, AbstractProtocol *parent)
{
    //qDebug("%s: &prev = %p &next = %p", __FUNCTION__, &prev, &next);
    mpStream = stream;
    this->parent = parent;
    prev = next = NULL;
    metaCount = -1;
    protoSize = -1;
}

/*!
  Destroys the abstract protocol
*/
AbstractProtocol::~AbstractProtocol()
{
}

/*! 
  Allocates and returns a new instance of the class. 
  
  Caller is responsible for freeing up after use.  Subclasses MUST implement 
  this function 
*/
AbstractProtocol* AbstractProtocol::createInstance(StreamBase* /* stream */,
    AbstractProtocol* /* parent */)
{
    return NULL;
}

/*!
  Returns the protocol's field number as defined in message 'Protocol', enum 'k'
  (file: protocol.proto)

  Subclasses MUST implement this function
*/
quint32 AbstractProtocol::protocolNumber() const
{
    qFatal("Something wrong!!!");
    return 0xFFFFFFFF;
}

/*!
  \fn virtual void AbstractProtocol::protoDataCopyInto(OstProto::Protocol &protocol) const = 0

  Copy the protocol's protobuf as an extension into the passed in protocol

  In the base class this is a pure virtual function. Subclasses MUST implement 
  this function. See the SampleProtocol for an example
*/


/*!
  \fn virtual void AbstractProtocol::protoDataCopyFrom(const OstProto::Protocol &protocol) = 0

  Copy and update the protocol's protobuf member data variable from the 
  passed in protocol

  In the base class this is a pure virtual function. Subclasses MUST implement 
  this function. See the SampleProtocol for an example
*/


/*!
  Returns the full name of the protocol

  The default implementation returns a null string 
*/
QString AbstractProtocol::name() const
{
    return QString(); 
}

/*!
  Returns the short name or abbreviation of the protocol

  The default implementation forms and returns an abbreviation composed
  of all the upper case chars in name() \n 
  The default implementation caches the abbreviation on its first invocation
  and subsequently returns the cached abbreviation 
*/
QString AbstractProtocol::shortName() const
{
    if (protoAbbr.isNull())
    {
        QString abbr;

        for (int i = 0; i < name().size(); i++)
            if (name().at(i).isUpper()) abbr.append(name().at(i));

        if (abbr.size())
            protoAbbr = abbr;
        else
            protoAbbr = QString("");
    }

    return protoAbbr;
}

/*! 
  Returns the number of fields in the protocol (both Frame fields and 
  Meta fields)

  The default implementation returns zero. Subclasses MUST implement this
  function.
*/
int AbstractProtocol::fieldCount() const
{
    return 0;
}

/*! 
  Returns the number of meta fields 

  The default implementation counts and returns the number of fields for which
  the FieldIsMeta flag is set\n
  The default implementation caches the count on its first invocation
  and subsequently returns the cached count 
*/
int AbstractProtocol::metaFieldCount() const
{
    if (metaCount < 0)
    {
        int c = 0;
        for (int i = 0; i < fieldCount() ; i++) 
            if (fieldFlags(i).testFlag(FieldIsMeta))
                c++;
        metaCount = c;
    }

    return metaCount;
}

/*! 
  Returns the number of frame fields

  Convenience method - same as fieldCount() minus metaFieldCount() 
*/
int AbstractProtocol::frameFieldCount() const
{
    //qDebug("%s:%d, %d", __FUNCTION__, fieldCount(), metaFieldCount());
    return (fieldCount() - metaFieldCount());
}

/*!
  Returns the field flags for the passed in field index

  The default implementation assumes all fields to be frame fields and returns
  'FieldIsNormal'. Subclasses must reimplement this method if they have any
  meta fields or checksum fields. See the SampleProtocol for an example.
*/
AbstractProtocol::FieldFlags AbstractProtocol::fieldFlags(int /*index*/) const
{
    return FieldIsNormal;
}

/*! 
  Returns the requested field attribute data

  Protocols which have meta fields that vary a frame field across
  streams may use the streamIndex to return the appropriate field value \n
  Some field attributes e.g. FieldName may be invariant across streams\n
  The FieldTextValue attribute may include additional information about
  the field's value e.g. a checksum field may include "(correct)" or 
  "(incorrect)" alongwith the actual checksum value. \n

  The default implementation returns a empty string for FieldName and 
  FieldTextValue; empty byte array of size 0 for FieldFrameValue; 0 for
  FieldValue; subclasses are expected to return meaning values for all
  these attributes. The only exception is the 'FieldBitSize' attribute - 
  the default implementation takes the (byte) size of FieldFrameValue,
  multiplies it with 8 and returns the result - this can be used by
  subclasses for fields which are an integral multiple of bytes; for
  fields whose size are a non-integral multiple of bytes or smaller than
  a byte, subclasses should return the correct value. Also for fields
  which represent checksums, subclasses should return a value for
  FieldBitSize - even if it is an integral multiple of bytes.

  \note If a subclass uses any of the below functions to derive 
  FieldFrameValue, the subclass should handle and return a value for 
  FieldBitSize to prevent endless recrusion -
  - protocolFrameCksum()
  - protocolFramePayloadSize()
*/
QVariant AbstractProtocol::fieldData(int index, FieldAttrib attrib,
        int streamIndex) const
{
    switch (attrib)
    {
        case FieldName:
            return QString();
        case FieldBitSize:
            Q_ASSERT_X(!fieldFlags(index).testFlag(FieldIsCksum),
                "AbstractProtocol::fieldData()",
                "FieldBitSize for checksum fields need to be handled by the subclass");
            return fieldData(index, FieldFrameValue, streamIndex).
                toByteArray().size() * 8;
        case FieldValue:
            return 0;
        case FieldFrameValue:
            return QByteArray();
        case FieldTextValue:
            return QString();

        default:
            qFatal("%s:%d: unhandled case %d\n", __FUNCTION__, __LINE__,
                    attrib);
    }

    return QVariant();
}

/*! 
  Sets the value of a field corresponding to index

  This method is called by the GUI code to store a user specified value into
  the protocol's protoBuf.  Currently this method is called with 
  FieldAttrib = FieldValue only.

  Returns true if field is successfully set, false otherwise.
  The default implementation always returns false. Subclasses should 
  reimplement this method. See SampleProtocol for an example.

*/
bool AbstractProtocol::setFieldData(int /*index*/, const QVariant& /*value*/,
        FieldAttrib /*attrib*/)
{
    return false;
}

/*!
  Returns the protocolIdType for the protocol

  The default implementation returns ProtocolIdNone. If a subclass has a
  protocolId field it should return the appropriate value e.g. IP protocol
  will return ProtocolIdIp, Ethernet will return ProtocolIdEth etc.
*/
AbstractProtocol::ProtocolIdType AbstractProtocol::protocolIdType() const
{
    return ProtocolIdNone;
}

/*!
  Returns the protocol id of the protocol for the given type

  The default implementation returns 0. If a subclass represents a protocol
  which has a particular protocol id, it should return the appropriate value.
  If a protocol does not have an id for the given type, it should defer to
  the base class. e.g. IGMP will return 2 for ProtocolIdIp, and defer to the
  base class for the remaining ProtocolIdTypes; IP will return 0x800 for 
  ProtocolIdEth type, 0x060603 for ProtocolIdLlc and 0x04 for ProtocolIdIp etc.
*/
quint32 AbstractProtocol::protocolId(ProtocolIdType /*type*/) const
{
    return 0;
}

/*!
  Returns the protocol id of the payload protocol (the protocol that 
  immediately follows the current one)

  A subclass which has a protocol id field, can use this to retrieve the
  appropriate value
*/
quint32 AbstractProtocol::payloadProtocolId(ProtocolIdType type) const
{
    quint32 id;

    if (next)
        id = next->protocolId(type);
    else if (parent)
        id = parent->payloadProtocolId(type);
    else
        id = 0xFFFFFFFF;

    qDebug("%s: payloadProtocolId = 0x%x", __FUNCTION__, id);
    return id;
}

/*!
  Returns the protocol's size in bytes

  The default implementation sums up the individual field bit sizes and
  returns it. The default implementation calculates the caches the size on
  the first invocation and subsequently returns the cached size.

  If the subclass protocol has a varying protocol size, it MUST reimplement
  this method, otherwise the default implementation is sufficient.
*/
int AbstractProtocol::protocolFrameSize(int streamIndex) const
{
    if (protoSize < 0)
    {
        int bitsize = 0;

        for (int i = 0; i < fieldCount(); i++)
        {
            if (!fieldFlags(i).testFlag(FieldIsMeta))
                bitsize += fieldData(i, FieldBitSize, streamIndex).toUInt();
        }
        protoSize = (bitsize+7)/8;
    }

    qDebug("%s: protoSize = %d", __FUNCTION__, protoSize);
    return protoSize;
}

/*!
  Returns the byte offset in the packet where the protocol starts

  This method is useful only for "padding" protocols i.e. protocols which
  fill up the remaining space for the user defined packet size e.g. the 
  PatternPayload protocol
*/
int AbstractProtocol::protocolFrameOffset(int streamIndex) const
{
    int size = 0;
    AbstractProtocol *p = prev;
    while (p)
    {
        size += p->protocolFrameSize(streamIndex);
        p = p->prev;
    }

    if (parent)
        size += parent->protocolFrameOffset(streamIndex);

    qDebug("%s: ofs = %d", __FUNCTION__, size);
    return size;
}

/*!
  Returns the size of the payload in bytes. The payload includes all protocols
  subsequent to the current

  This method is useful for protocols which need to fill in a payload size field
*/
int AbstractProtocol::protocolFramePayloadSize(int streamIndex) const
{
    int size = 0;
    AbstractProtocol *p = next;
    while (p)
    {
        size += p->protocolFrameSize(streamIndex);
        p = p->next;
    }
    if (parent)
        size += parent->protocolFramePayloadSize(streamIndex);

    qDebug("%s: payloadSize = %d", __FUNCTION__, size);
    return size;
}


/*! 
  Returns a byte array encoding the protocol (and its fields) which can be
  inserted into the stream's frame

  The default implementation forms and returns an ordered concatenation of 
  the FrameValue of all the 'frame' fields of the protocol also taking care of 
  fields which are not an integral number of bytes\n 
*/
QByteArray AbstractProtocol::protocolFrameValue(int streamIndex, bool forCksum) const
{
    QByteArray proto, field;
    uint bits, lastbitpos = 0;
    FieldFlags flags;

    for (int i=0; i < fieldCount() ; i++) 
    {
        flags = fieldFlags(i);
        if (!flags.testFlag(FieldIsMeta))
        {
            bits = fieldData(i, FieldBitSize, streamIndex).toUInt();
            if (bits == 0)
                continue;
            Q_ASSERT(bits > 0);

            if (forCksum && flags.testFlag(FieldIsCksum))
            {
                field.resize((bits+7)/8);
                field.fill('\0');
            }
            else
                field = fieldData(i, FieldFrameValue, streamIndex).toByteArray();
            qDebug("<<< %d, %d/%d >>>>", proto.size(), bits, field.size());

            if (bits == (uint) field.size() * 8)
            {
                if (lastbitpos == 0)
                    proto.append(field);
                else
                {
                    Q_ASSERT(field.size() > 0);

                    char c = proto[proto.size() - 1];
                    proto[proto.size() - 1] =  c | (field.at(0) >> lastbitpos);
                    for (int j = 0; j < field.size() - 1; j++)
                        proto.append(field.at(j) << lastbitpos |
                                field.at(j+1) >> lastbitpos);
                }
            }
            else if (bits < (uint) field.size() * 8)
            {
                uchar c;
                uint v;

                v = (field.size()*8) - bits;

                Q_ASSERT(v < 8);

                if (lastbitpos == 0)
                {
                    for (int j = 0; j < field.size(); j++)
                    {
                        c = field.at(j) << v;
                        if ((j+1) < field.size())
                            c |= ((uchar)field.at(j+1) >> (8-v));
                        proto.append(c);
                    }

                    lastbitpos = (lastbitpos + bits) % 8;
                }
                else
                {
                    Q_ASSERT(proto.size() > 0);

                    for (int j = 0; j < field.size(); j++)
                    {
                        uchar d;

                        c = field.at(j) << v;
                        if ((j+1) < field.size())
                            c |= ((uchar) field.at(j+1) >> (8-v));
                        d = proto[proto.size() - 1];
                        proto[proto.size() - 1] = d | ((uchar) c >> lastbitpos);
                        if (bits > (8*j + (8 - v)))
                            proto.append(c << (8-lastbitpos));
                    }

                    lastbitpos = (lastbitpos + bits) % 8;
                }
            }
            else // if (bits > field.size() * 8)
            {
                qFatal("bitsize more than FrameValue size. skipping...");
                continue;
            }
        }
    }

    return proto;
}

/*!
  Returns true if the protocol varies one or more of its fields at run-time,
  false otherwise

  The default implementation returns false. A subclass should reimplement
  if it has varying fields e.g. an IP protocol that increments/decrements
  the IP address with every packet  
*/
bool AbstractProtocol::isProtocolFrameValueVariable() const
{
    return false;
}

/*!
  Returns true if the protocol varies its size at run-time, false otherwise

  The default implmentation returns false. A subclass should reimplement
  if it varies its size at run-time e.g. a Payload protocol for a stream with
  incrementing/decrementing frame lengths
*/
bool AbstractProtocol::isProtocolFrameSizeVariable() const
{
    return false;
}

/*!
  Returns true if the payload content for a protocol varies at run-time,
  false otherwise

  This is useful for subclasses which have fields dependent on payload content 
  (e.g. UDP has a checksum field that varies if the payload varies)
*/
bool AbstractProtocol::isProtocolFramePayloadValueVariable() const
{
    AbstractProtocol *p = next;

    while (p)
    {
        if (p->isProtocolFrameValueVariable())
            return true;
        p = p->next;
    }
    if (parent && parent->isProtocolFramePayloadValueVariable())
        return true;

    return false;
}

/*!
  Returns true if the payload size for a protocol varies at run-time,
  false otherwise

  This is useful for subclasses which have fields dependent on payload size 
  (e.g. UDP has a checksum field that varies if the payload varies)
*/
bool AbstractProtocol::isProtocolFramePayloadSizeVariable() const
{
    AbstractProtocol *p = next;

    while (p)
    {
        if (p->isProtocolFrameSizeVariable())
            return true;
        p = p->next;
    }
    if (parent && parent->isProtocolFramePayloadSizeVariable())
        return true;

    return false;
}

/*!
  Returns the checksum (of the requested type) of the protocol's contents

  Useful for protocols which have a checksum field 

  \note If a subclass uses protocolFrameCksum() from within fieldData() to
  derive a cksum field, it MUST handle and return the 'FieldBitSize'
  attribute also for that particular field instead of using the default
  AbstractProtocol implementation for 'FieldBitSize' - this is required 
  to prevent infinite recursion
*/
quint32 AbstractProtocol::protocolFrameCksum(int streamIndex,
    CksumType cksumType) const
{
    static int recursionCount = 0;
    quint32 cksum = 0xFFFFFFFF;

    recursionCount++;
    Q_ASSERT_X(recursionCount < 10, "protocolFrameCksum", "potential infinite recursion - does a protocol checksum field not implement FieldBitSize?");

    switch(cksumType)
    {
        case CksumIp:
        {
            QByteArray fv;
            quint16 *ip;
            quint32 len, sum = 0;

            fv = protocolFrameValue(streamIndex, true);
            ip = (quint16*) fv.constData();
            len = fv.size();

            while(len > 1)
            {
                sum += *ip;
                if(sum & 0x80000000)
                    sum = (sum & 0xFFFF) + (sum >> 16);
                ip++;
                len -= 2;
            }

            if (len)
                sum += (unsigned short) *(unsigned char *)ip;

            while(sum>>16)
                sum = (sum & 0xFFFF) + (sum >> 16);

            cksum = qFromBigEndian((quint16) ~sum);
            break;
        }

        case CksumTcpUdp:
        {
            quint16 cks;
            quint32 sum = 0;

            cks = protocolFrameCksum(streamIndex, CksumIp);
            sum += (quint16) ~cks;
            cks = protocolFramePayloadCksum(streamIndex, CksumIp);
            sum += (quint16) ~cks;
            cks = protocolFrameHeaderCksum(streamIndex, CksumIpPseudo);
            sum += (quint16) ~cks;

            while(sum>>16)
                sum = (sum & 0xFFFF) + (sum >> 16);

            cksum = (~sum) & 0xFFFF;
            break;
        }    
        default:
            break;
    }

    recursionCount--;
    return cksum;
}

/*!
  Returns the checksum of the requested type for the protocol's header

  This is useful for subclasses which needs the header's checksum e.g. TCP/UDP
  require a "Pseudo-IP" checksum

  Currently the default implementation supports only type CksumIpPseudo
*/
quint32 AbstractProtocol::protocolFrameHeaderCksum(int streamIndex, 
    CksumType cksumType) const
{
    quint32 sum = 0;
    quint16 cksum;
    AbstractProtocol *p = prev;

    Q_ASSERT(cksumType == CksumIpPseudo);

    while (p)
    {
        cksum = p->protocolFrameCksum(streamIndex, cksumType);
        sum += (quint16) ~cksum;
        p = p->prev;
        qDebug("%s: sum = %u, cksum = %u", __FUNCTION__, sum, cksum);
    }
    if (parent)
    {
        cksum = parent->protocolFrameHeaderCksum(streamIndex, cksumType);
        sum += (quint16) ~cksum;
    }

    while(sum>>16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (quint16) ~sum;
}

/*!
  Returns the checksum of the requested type for the protocol's payload

  This is useful for subclasses which needs the payload's checksum e.g. TCP/UDP
  require a IP checksum of the payload (to be combined with other checksums to
  derive the final checksum)

  Currently the default implementation supports only type CksumIp
*/
quint32 AbstractProtocol::protocolFramePayloadCksum(int streamIndex,
    CksumType cksumType) const
{
    quint32 sum = 0;
    quint16 cksum;
    AbstractProtocol *p = next;

    Q_ASSERT(cksumType == CksumIp);

    while (p)
    {
        cksum = p->protocolFrameCksum(streamIndex, cksumType);
        sum += (quint16) ~cksum;
        p = p->next;
    }

    if (parent)
    {
        cksum = parent->protocolFramePayloadCksum(streamIndex, cksumType);
        sum += (quint16) ~cksum;
    }

    while(sum>>16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (quint16) ~sum;
}

/*!
  \fn virtual QWidget* AbstractProtocol::configWidget() = 0;

  Returns the configuration widget for the protocol. The protocol retains
  ownership of the configuration widget - the caller should not free it.

  In the base class this is a pure virtual function. Subclasses MUST implement 
  this function. See the SampleProtocol for an example
*/

/*!
  \fn virtual void AbstractProtocol::loadConfigWidget() = 0;

  Loads data from the protocol's protobuf into the configuration widget of
  the protocol

  In the base class this is a pure virtual function. Subclasses MUST implement 
  this function. See the SampleProtocol for an example
*/

/*!
  \fn virtual void AbstractProtocol::storeConfigWidget() = 0;

  Stores data from the configuration widget of the protocol into the protocol's
  protobuf

  In the base class this is a pure virtual function. Subclasses MUST implement 
  this function. See the SampleProtocol for an example
*/

