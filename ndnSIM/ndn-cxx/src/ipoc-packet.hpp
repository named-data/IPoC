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

#ifndef NDN_IPOC_PACKET_HPP 
#define NDN_IPOC_PACKET_HPP

#include "encoding/encoding-buffer.hpp"

namespace ndn {

class IPoCPacket
{
public:
    class Error : public tlv::Error
    {
    public:
        explicit
        Error(const std::string& what)
            : tlv::Error(what)
        {
        }
    };

    enum Type {
        IPoCPacket_ControlBits = 1,
        IPoCPacket_SequenceNumber = 2
    };

public: // constructors
    IPoCPacket();
    
    explicit
    IPoCPacket(const Block& wire);

public: // encode and decode
    void
    wireDecode(const Block& wire);

    template<encoding::Tag TAG>
    size_t
    wireEncode(EncodingImpl<TAG>& encoder) const;

    const Block&
    wireEncode() const;


public: // attributes
    IPoCPacket&
    setPayload(const Block& payload);
    
    const Block&
    getPayload();

    IPoCPacket&
    setControlBits(const uint8_t controlBits);
    
    const uint8_t
    getControlBits();
    
    IPoCPacket&
    setSequenceNumber(const uint64_t seqNumber);

    const uint64_t
    getSequenceNumber();


private:
    Block m_payload;
    uint8_t m_controlBits;
    uint64_t m_seqNumber;
    
    mutable Block m_wire;
};

} // namespace ndn

#endif
