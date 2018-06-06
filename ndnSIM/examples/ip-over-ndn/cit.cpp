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

#include "cit.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.Cit");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(Cit);

TypeId
Cit::GetTypeId()
{
  static TypeId tid = TypeId("ns3::ndn::Cit").SetGroupName("Ndn").SetParent<Object>();
  return tid;
}

uint32_t
Cit::GetId() const
{
  return m_id;
}

Cit::Cit()
    : m_max(1)
    , m_min(0)
{
}

void
Cit::setPrefix(Name& prefix)
{
    m_prefix = prefix;
}

void
Cit::setEntryParams(uint32_t max, uint32_t min)
{
    m_max = max;
    m_min = min;
}

shared_ptr<CitEntry>
Cit::findOrInsertCitEntry(const Name& cltAddr)
{
    shared_ptr<CitEntry> entry;

    auto find = m_entryList.find(cltAddr);
    if (find == m_entryList.end()) {
        NS_LOG_DEBUG("fail to find " << cltAddr);
        entry = make_shared<CitEntry>(cltAddr, m_max, m_min);
        m_entryList.insert(std::make_pair(cltAddr, entry));
    } else {
        NS_LOG_DEBUG("succeed to find " << cltAddr);
        entry = find->second;
    }
    return entry;
}

} // namespace ns3
} // namespace ndn
