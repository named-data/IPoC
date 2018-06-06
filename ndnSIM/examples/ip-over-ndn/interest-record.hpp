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

#ifndef CLIENT_INTEREST_RECORD_HPP
#define CLIENT_INTEREST_RECORD_HPP

#include "ns3/ndnSIM/model/ndn-common.hpp"

namespace ns3 {

namespace ndn {

class InterestRecord
{
public:
    explicit
    InterestRecord(uint64_t seqNu);

    uint64_t
    getSeqNumber();

public:
    EventId m_timeoutId;

private:
    uint64_t m_seqNu;
};

} // namespace ndn
} // namespace ns3

#endif // CLIENT_INTEREST_RECORD_HPP
