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

#ifndef CLIENT_INTEREST_TABLE_HPP
#define CLIENT_INTEREST_TABLE_HPP

#include "cit-entry.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/object.h"

namespace ns3 {
namespace ndn {

class Cit : public Object
{
public:
    /**
     * \brief Interface ID
     *
     * \return interface ID
     */
    static TypeId
    GetTypeId();

    /**
     * @brief Get numeric ID of the node (internally assigned)
     */
    uint32_t
    GetId() const;

    explicit
    Cit();

    void
    setPrefix(Name& prefix);

    void
    setEntryParams(uint32_t max, uint32_t min);

    shared_ptr<CitEntry>
    findOrInsertCitEntry(const Name& cltAddr);

private:
    uint32_t m_id;
    uint32_t m_max;
    uint32_t m_min;
    std::unordered_map<Name, std::shared_ptr<CitEntry>> m_entryList;
    Name m_prefix;
};

} // namespace ndn

} // namespace ns3

#endif // CLIENT_INTEREST_TABLE_HPP
