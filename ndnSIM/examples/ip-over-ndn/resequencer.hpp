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

#ifndef RESEQUENCER_HPP
#define RESEQUENCER_HPP

#include <boost/circular_buffer.hpp>
#include <boost/tuple/tuple.hpp>

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/log.h"

#include <queue>
#include <map>
#include <functional>
#include <type_traits>

namespace ns3 {
namespace ndn {

template <typename T>
class Compare
{
public:
    bool operator() (T l, T r);
};

template <typename T>
bool
Compare<T>::operator() (T l, T r)
{
    return true;
}

template <>
bool
Compare<shared_ptr<const Data>>::operator() (shared_ptr<const Data> l, shared_ptr<const Data> r);

template <>
bool
Compare<shared_ptr<const Interest>>::operator() (shared_ptr<const Interest> l, shared_ptr<const Interest> r);


class CompareEID
{
public:
    bool operator() (ns3::EventId l, ns3::EventId r)
    {
        return l.GetUid() > r.GetUid(); //test this
    }
};


/** \brief packet resequencer at the IpoC client
 */

template <typename T>
class Resequencer : public Object
{

public:
    typedef std::function<void (shared_ptr<std::vector<T>>)> CallbackFunc;

    /**
     * \brief Interface ID
     *
     * \return interface ID
     */
    static TypeId
    GetTypeId()
    {
        static TypeId tid =
        TypeId("ns3::ndn::Resequencer")
          .SetGroupName("ndn")
          .SetParent<Object>()
        ;
        return tid;
    }

    /**
     * @brief Get numeric ID of the node (internally assigned)
     */
    uint32_t
    GetId() const
    {
        return m_id;
    }

    Resequencer();

    uint32_t GetWaitForGap(void) {
        return m_waitForGapAtReseq;
    }

    void SetWaitForGap(uint32_t gap_duration) {
        //std::cout << "************Wait for Gap called " << gap_duration << std::endl;
        m_waitForGapAtReseq = gap_duration;
        //std::cout << "************Wait for Gap called " << m_waitForGapAtReseq << std::endl;
    }

    uint32_t GetLastUpstreamSentSeq(void)
    {
        return m_lastSentUpstreamSeq;
    }

    void SetLastUpstreamSentSeq(uint32_t this_seq)
    {
        m_lastSentUpstreamSeq = this_seq;
    }

    void setMaxEncounteredSeq(uint32_t thisSeq) {
        m_maxEncounteredSeq = thisSeq;
    }

    uint32_t getMaxEncounteredSeq(void) {
        return m_maxEncounteredSeq;
    }

    void setLastSeq(uint32_t lastSeq) {
        m_lastSentSequence = lastSeq;
    }

    uint32_t getLastSeq(void) {
        return m_lastSentSequence;
    }

    void insert(T pkt) {
		    m_storage.push(pkt);
    }

    uint32_t getSequenceNumber(T pkt);

    uint32_t getTopIpocSequence()
    {
        if (!m_storage.empty()) {
            auto pkt = m_storage.top();
            return getSequenceNumber(pkt);
        }
        else {
            return -1;
        }
    }

    bool
    isStorageEmpty() {
        return m_storage.empty();
    }

    T retrieveIpocPacket() {
        if (m_storage.empty()) {
            BOOST_THROW_EXCEPTION(std::range_error("Empty storage"));
        }
        auto pkt = m_storage.top();
        m_storage.pop();
        return pkt;
    }

    void pushToEventRecorder(ns3::EventId eventID) {
        m_pq.push(eventID);
    }

    void removeAllPreviousEvents(uint32_t this_seq){
        if(!m_pq.empty()) {
            while(m_pq.top().GetUid() < this_seq)
            {
                auto eventId = m_pq.top();
                //std::cout << " Removing event ID " << eventId.GetUid()  << std::endl;
                Simulator::Remove(eventId);
                m_pq.pop();
            }
        }
    }

    void
    releasePartialQueue(shared_ptr<std::vector<T>> pktls);

    void
    forwardOrQueue(T pkt, const CallbackFunc& callbackFunc);

    void
    sendDataQueue(uint32_t this_seq, const CallbackFunc& callbackFunc);

private:
    //resequencer as a circular fifo buffer
    uint32_t m_lastSentSequence= 0;
    uint32_t m_maxEncounteredSeq = 0;
    uint32_t m_lastSentUpstreamSeq = 0;
    uint32_t m_waitForGapAtReseq;
    uint32_t m_citTableWait;
    bool m_isInterest;
    std::priority_queue<T, std::vector<T>, Compare<T>> m_storage;
    std::priority_queue<ns3::EventId, std::vector<ns3::EventId>, CompareEID> m_pq;
    uint32_t m_id;
};

template <typename T>
Resequencer<T>::Resequencer()
{
    if (std::is_same<T, shared_ptr<const Data>>::value) {
        m_isInterest = false;
    } else {
        m_isInterest = true;
    }
}

template <typename T>
uint32_t
Resequencer<T>::getSequenceNumber(T pkt)
{
    return 0;
}

template <>
uint32_t
Resequencer<shared_ptr<const Data>>::getSequenceNumber(shared_ptr<const Data> pkt);

template <>
uint32_t
Resequencer<shared_ptr<const Interest>>::getSequenceNumber(shared_ptr<const Interest> pkt);

template <typename T>
void
Resequencer<T>::sendDataQueue(uint32_t this_seq, const CallbackFunc& callbackFunc) {
    //send everything within gap_min and gap_max, in order
    //std::cout << "Timer expired: releasing everything upto sequence " << this_seq << std::endl;
    auto lastSentSeq = getLastSeq();
    auto maxEncounteredSeq = getMaxEncounteredSeq();
    //std::cout<< "!!! This Seq " << this_seq << " lastSentSeq " << lastSentSeq << " Max Segment " << maxEncounteredSeq << std::endl;


    auto resPktls = make_shared<std::vector<T>>();
    while ((getTopIpocSequence() <= this_seq) or (getTopIpocSequence() - getLastSeq() == 1)) {
        if (isStorageEmpty())
            break;
        auto pkt = retrieveIpocPacket();
        if (pkt != nullptr) {
            resPktls->push_back(pkt);
            auto x = getSequenceNumber(pkt);
            if (x > getLastSeq())
              setLastSeq(x);
        }
    }
    //cancel all timers before this, used a priority queue
    removeAllPreviousEvents(this_seq);
    callbackFunc(resPktls);
}

template <typename T>
void
Resequencer<T>::releasePartialQueue(shared_ptr<std::vector<T>> pktls)
{
    //send consequtive elements starting from top
    //std::cout << "In partial release queue" << std::endl;
    while (getTopIpocSequence() - getLastSeq() == 1) {
        auto x = getTopIpocSequence();
        auto data_x = retrieveIpocPacket();
        if (data_x != nullptr) {
            // get Interests need to forward
            pktls->push_back(data_x);
            setLastSeq(x);
        }
    }
}

template <typename T>
void
Resequencer<T>::forwardOrQueue(T pkt, const CallbackFunc& callbackFunc)
{
    Block blk = pkt->wireEncode();
    // TODO: figure out how to use the tlv type name
    //if (m_isInterest) {
    //    std::cout << "---- Resequencer<T>::forwardOrQueue: Interest in Resequencer BEGIN ----" << std::endl;
    //} else {
    //    std::cout << "---- Resequencer<T>::forwardOrQueue: Data in Resequencer BEGIN ----" << std::endl;
    //}

    //std::cout << "Pakcet name " << pkt->getName() << std::endl;
    try {
        pkt->getName();
    }
    catch (const std::exception& e) {
        std::cout << "Having trouble to get Name for packet" << std::endl;
    }

    auto pktls = make_shared<std::vector<T>>();
    // TODO: get sequencer number method
    auto thisSeq = getSequenceNumber(pkt);
    auto lastSentSeq = getLastSeq();
    auto maxEncounteredSeq = getMaxEncounteredSeq();
    //std::cout<< "###This Seq " << thisSeq << " lastSentSeq " << lastSentSeq << " Max Segment " << maxEncounteredSeq << std::endl;

    if(thisSeq < lastSentSeq) {
        //std::cout << "I am late! Drop me. Seq: " << thisSeq << std::endl;
        return;
    }

    bool maxSegmentFlag = false;
    if (thisSeq > maxEncounteredSeq) {
        setMaxEncounteredSeq(thisSeq);
        maxSegmentFlag = true;
    }

    if (thisSeq - lastSentSeq == 1 || (thisSeq == 0 && lastSentSeq == 0)) { //don't queue, send
        //std::cout << "This is the expected packet, just forward" << std::endl;
        // get Interests need to forward
        pktls->push_back(pkt);
        setLastSeq(thisSeq);

        //is this the max segment? pass
        //else release partial queue
        if (!maxSegmentFlag && thisSeq != 0) {
            //std::cout << "This packet is not the higest sequence number we have seen, try to do a partial release on queue" << std::endl;
            releasePartialQueue(pktls);
        }
        callbackFunc(pktls);
    } //else packet is either creating a gap, or is in a gap, or filling  a gap
    else {
        insert(pkt);
        auto gap_length = thisSeq - lastSentSeq;
        //auto gap_duration = gap_length*GetWaitForGap();
        auto gap_duration = GetWaitForGap();
        //std::cout << "$$$$$Gap found, inserting new sequence into the queue : " << thisSeq << " wait for gap " << gap_duration << " microseconds " << std::endl;

        //No! Gap still exists, insert into buffer, schedule a timer
        //when the timer expires, release everything upto this point
        auto eventID = Simulator::Schedule(MicroSeconds(gap_duration), &Resequencer<T>::sendDataQueue, this, thisSeq, callbackFunc);
        //insert in a map, smallest event ID is at the top
        pushToEventRecorder(eventID);
        //std::cout << "Scheduling event ID : " << eventID.GetUid() << std::endl;
    }

    //if (m_isInterest) {
    //    std::cout << "---- Resequencer<T>::forwardOrQueue: Interest in Resequencer END ----" << std::endl;
    //} else {
    //    std::cout << "---- Resequencer<T>::forwardOrQueue: Data in Resequencer END ----" << std::endl;
    //}
}

} // namespace ndn
} // namespace ns3

#endif // //end RESEQUENCER_HPP
