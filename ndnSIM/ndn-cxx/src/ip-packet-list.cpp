/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 */

#include "ip-packet-list.hpp"

namespace ndn {

IpPacketList::Error::Error(const std::string& what)
  : tlv::Error(what)
{
}

IpPacketList::Error::Error(const std::string& what, const std::exception& innerException)
  : Error(what + std::string(": ") + innerException.what())
{
}

IpPacketList::IpPacketList()
{
}

IpPacketList::IpPacketList(std::initializer_list<Block> pktls)
{
    for (const Block& pkt : pktls) {
        m_pktls.push_back(pkt);
    }
}

IpPacketList::IpPacketList(const Block& block)
{
    this->wireDecode(block);
}

void
IpPacketList::addIpPacketBlock(const Block& block)
{
    m_pktls.push_back(block);
}

template<encoding::Tag TAG>
size_t
IpPacketList::wireEncode(EncodingImpl<TAG>& encoder, uint32_t type) const
{
    if (type != tlv::IpPacketList) {
        BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid TLV-TYPE " + to_string(type) +
                                                  " when encoding IpPacketList"));
    }
    
    if (this->size() == 0) {
        BOOST_THROW_EXCEPTION(Error("Empty IpPacketList"));
    }
    
    // IpPacketList ::= (type) TLV-LENGTH
    //                    IpPacket+
    
    // IpPacket ::= IP-PACKET-TYPE TLV-LENGTH
    //                  IP-PACKET
    
    size_t totalLen = 0;
    for (auto i = m_pktls.rbegin(); i != m_pktls.rend(); ++i) {
        totalLen += encoder.prependBlock(*i);
    }
    totalLen += encoder.prependVarNumber(totalLen);
    totalLen += encoder.prependVarNumber(type);
    return totalLen;
}

template size_t
IpPacketList::wireEncode<encoding::EncoderTag>(EncodingImpl<encoding::EncoderTag>&, uint32_t) const;

template size_t
IpPacketList::wireEncode<encoding::EstimatorTag>(EncodingImpl<encoding::EstimatorTag>&, uint32_t) const;

const Block&
IpPacketList::wireEncode() const
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
IpPacketList::wireDecode(const Block& block)
{
    if (block.type() != tlv::IpPacketList) {
        BOOST_THROW_EXCEPTION(std::invalid_argument("Unexpected TLV-TYPE " + to_string(block.type()) +
                                                  " when decoding IpPacketList"));
    }
    
    m_pktls.clear();
    
    block.parse();
    for (const auto& pkt : block.elements()) {
        if (pkt.type() != tlv::IpPacket) {
            BOOST_THROW_EXCEPTION(Error("Unexpected TLV-TYPE " + to_string(pkt.type()) +
                                        " when decoding IP Packet"));
        }
        m_pktls.push_back(pkt);
    }
    
    if (this->size() == 0) {
        BOOST_THROW_EXCEPTION(Error("Empty IpPacketList"));
    }
}

} // namespace ndn
