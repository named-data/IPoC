/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
*
* Copyright (c) 2017 Cable Television Laboratories, Inc.
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation;
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
* Authors: Susmit Shannigrahi <susmit@colostate.edu>
           Chengyu Fan <chengyu.fan@colostate.edu>
           Greg White <g.white@cablelabs.com>
*
*/
 
#include "ipoc-packet.hpp"
#include "encoding/block-helpers.hpp"

namespace ndn {

IPoCPacket::IPoCPacket()
    : m_controlBits(0)
{
}

IPoCPacket::IPoCPacket(const Block& wire)
{
    wireDecode(wire);
}

template<encoding::Tag TAG>
size_t
IPoCPacket::wireEncode(EncodingImpl<TAG>& encoder) const
{
    // IPoCPacket ::= Conten_type TLV-LENGTH IPoC_controlBits IPoC_payload
    // IPoC_controlBits ::= type length controlBits
    // IPoC_payload ::= type length payload
    
    size_t totalLength = 0;
    
    if (m_payload.value_size() != 0) {
        totalLength += encoder.prependBlock(m_payload);
    }
    
    totalLength += prependNonNegativeIntegerBlock(encoder, IPoCPacket_SequenceNumber, m_seqNumber);
    totalLength += prependNonNegativeIntegerBlock(encoder, IPoCPacket_ControlBits, m_controlBits);
    
    totalLength += encoder.prependVarNumber(totalLength);
    // Use AppPrivateBlock1 as the IPoCPacket (control bits and payload)
    totalLength += encoder.prependVarNumber(tlv::Content);
    return totalLength;
}

template size_t
IPoCPacket::wireEncode<encoding::EncoderTag>(EncodingImpl<encoding::EncoderTag>& encoder) const;

template size_t
IPoCPacket::wireEncode<encoding::EstimatorTag>(EncodingImpl<encoding::EstimatorTag>& encoder) const;

const Block&
IPoCPacket::wireEncode() const
{
    if (m_wire.hasWire())
        m_wire.reset();
    
    EncodingEstimator estimator;
    size_t estimatedSize = wireEncode(estimator);
    
    EncodingBuffer buffer(estimatedSize, 0);
    wireEncode(buffer);
    
    m_wire = buffer.block();
    return m_wire;
}

void
IPoCPacket::wireDecode(const Block& wire)
{
    if (wire.type() != tlv::Content)
        BOOST_THROW_EXCEPTION(Error("Unexpected TLV type during IPoCPacket decoding"));
    
    m_wire = wire;
    m_wire.parse();
    
    Block::element_const_iterator val = m_wire.elements_begin();
    
    if (val != m_wire.elements_end() && val->type() == IPoCPacket_ControlBits) {
        m_controlBits = static_cast<uint8_t>(readNonNegativeInteger(*val));
        ++val;
    }
    
    if (val != m_wire.elements_end() && val->type() == IPoCPacket_SequenceNumber) {
        m_seqNumber = readNonNegativeInteger(*val);
        ++val;
    }

    if (val != m_wire.elements_end() && val->type() == tlv::IpPacketList) {
        m_payload = *val;
        ++val;
    }

}

IPoCPacket&
IPoCPacket::setPayload(const Block& payload)
{
    if (payload.type() != tlv::IpPacketList)
        BOOST_THROW_EXCEPTION(Error("expecting IpPacket block"));
    
    m_payload.reset();
    m_payload = payload;
    return *this;
}

const Block&
IPoCPacket::getPayload()
{
    return m_payload;
}

IPoCPacket&
IPoCPacket::setControlBits(uint8_t controlBits)
{
    m_controlBits = 0;
    m_controlBits = controlBits;
    return *this;
}

const uint8_t
IPoCPacket::getControlBits()
{
    return m_controlBits;
}

IPoCPacket&
IPoCPacket::setSequenceNumber(uint64_t seqNumber)
{
    m_seqNumber = 0;
    m_seqNumber = seqNumber;
    return *this;
}

const uint64_t
IPoCPacket::getSequenceNumber()
{
    return m_seqNumber;
}
} // namespace ndn
