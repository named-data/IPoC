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

#include "resequencer.hpp"

namespace ns3 {
namespace ndn {


template <>
bool
Compare<std::shared_ptr<const Data>>::operator() (std::shared_ptr<const Data> l, std::shared_ptr<const Data> r)
{
    IPoCPacket lPkt(l->getContent());
    IPoCPacket rPkt(r->getContent());
    auto lSeq = lPkt.getSequenceNumber();
    auto rSeq = rPkt.getSequenceNumber();
    return lSeq > rSeq; //test this
}

template <>
bool
Compare<std::shared_ptr<const Interest>>::operator() (std::shared_ptr<const Interest> l, std::shared_ptr<const Interest> r)
{
    auto lSeq = l->getName().get(-1).toSegment();
    auto rSeq = r->getName().get(-1).toSegment();
    return lSeq > rSeq; //test this
}

template <>
uint32_t
Resequencer<std::shared_ptr<const Data>>::getSequenceNumber(std::shared_ptr<const Data> pkt)
{
	IPoCPacket Pkt(pkt->getContent());
	auto Seq = Pkt.getSequenceNumber();
	return (Seq);
}

template <>
uint32_t
Resequencer<std::shared_ptr<const Interest>>::getSequenceNumber(std::shared_ptr<const Interest> pkt)
{
    return pkt->getName().get(-1).toSegment();
}

} // namespace ndn
} // namespace ns3
