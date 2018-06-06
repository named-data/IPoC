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

#ifndef NDN_IP_PACKET_LIST_HPP
#define NDN_IP_PACKET_LIST_HPP

#include "encoding/encoding-buffer.hpp"
#include <initializer_list>

namespace ndn {

/** \brief represents a list of IP packets
 *
 *  IP packets are stored in an std::vector, under the assumption that there is usually only a
 *  small number of IP packets, so that copying is acceptable when they are modified.
 */
class IpPacketList
{
public:
    class Error : public tlv::Error
    {
    public:
        explicit
        Error(const std::string& what);
        
        Error(const std::string& what, const std::exception& innerException);
    };

    IpPacketList();
    
    IpPacketList(std::initializer_list<Block> pktls);
    
    explicit
    IpPacketList(const Block& block);
    
    void
    addIpPacketBlock(const Block& block);
    
    template<encoding::Tag TAG>
    size_t
    wireEncode(EncodingImpl<TAG>& encoder, uint32_t type = tlv::IpPacketList) const;
    
    void
    wireDecode(const Block& block);
    
    
    const Block&
    wireEncode() const;
    
    using const_iterator = std::vector<Block>::const_iterator;
    
    const_iterator
    begin() const
    {
      return m_pktls.begin();
    }
    
    const_iterator
    end() const
    {
      return m_pktls.end();
    }
    
    size_t
    size() const
    {
      return m_pktls.size();
    }

private:
    std::vector<Block> m_pktls;
    mutable Block m_wire;
};

} // namespace ndn

#endif
