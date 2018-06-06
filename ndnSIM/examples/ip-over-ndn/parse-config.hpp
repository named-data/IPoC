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
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/exception/all.hpp>
#include <boost/exception/error_info.hpp>

#include <exception>

class ParseConfig 
{

typedef struct application_conf {
  std::string bytesList;
  std::string timeIntervalList;
} AppConf;

boost::property_tree::ptree pt;
public:
    void setVariables();
    ParseConfig(const std::string& configFile);

public:
    std::string p_name;
    uint32_t p_timer0=0;
    uint32_t p_timer1=0;
    uint32_t p_MaxIDC=0;
    uint32_t p_reseqLen=0;
    uint32_t p_waitForGap=0;
    std::string p_prefix;
    uint32_t p_payloadSize = 0;
    uint32_t p_contentFreshness = 0;
    uint32_t p_citTableWait = 0;
    uint32_t p_maxCitEntrySize = 0;
    uint32_t p_minCitEntrySize = 0;

    //sender
    uint32_t p_sendSize = 0;
    uint32_t p_maxBytes = 0;
    //std::string p_maxBytesList;
    //std::string p_timeIntervalList;
    std::vector<AppConf> p_apps;
    float p_simulationErrorRate;
};


ParseConfig::ParseConfig(const std::string& configFile) {
    boost::property_tree::info_parser::read_info(configFile, pt);
}

void ParseConfig::setVariables() {
    if (pt.empty()) {
        BOOST_THROW_EXCEPTION(std::runtime_error("Empty or Malformed Config File"));
    }

    for (auto it:pt) {
        auto section = it.first;
        //NS3_LOG_DEBUG("Parsing Section " << section);
        std::cout << "Parsing Section " << section << std::endl;
        const boost::property_tree::ptree &pt2 = it.second;
        if (section == "RequestHelper") {
            p_name = pt2.get<std::string>("name");
            p_timer0 = pt2.get<uint32_t>("timer0");
            p_timer1 = pt2.get<uint32_t>("timer1");
            p_MaxIDC = pt2.get<uint32_t>("maxidc");
            p_reseqLen = pt2.get<uint32_t>("reseqlen");
            p_waitForGap = pt2.get<uint32_t>("waitforgap");

            BOOST_ASSERT(!p_name.empty());
            BOOST_ASSERT(p_timer0 != 0);
            BOOST_ASSERT(p_timer1 != 0);
            BOOST_ASSERT(p_MaxIDC != 0);
            BOOST_ASSERT(p_reseqLen != 0);
            BOOST_ASSERT(p_waitForGap != 0);
        }
        else if (section == "ProducerHelper") {
            p_prefix = pt2.get<std::string>("prefix");
            p_payloadSize = pt2.get<uint32_t>("payloadsize");
            p_contentFreshness = pt2.get<uint32_t>("contentfreshness");
            p_citTableWait = pt2.get<uint32_t>("cittablewait");
            p_maxCitEntrySize = pt2.get<uint32_t>("maxCitEntrySize");
            p_minCitEntrySize = pt2.get<uint32_t>("minCitEntrySize");

            BOOST_ASSERT(!p_prefix.empty());
            BOOST_ASSERT(p_payloadSize != 0);
            BOOST_ASSERT(p_contentFreshness != 0);
            BOOST_ASSERT(p_waitForGap != 0);
            BOOST_ASSERT(p_maxCitEntrySize != 0);
            BOOST_ASSERT(p_minCitEntrySize != 0);
        }
        else if (section == "Sender") {
            for (auto it2:pt2) {
                auto section2 = it2.first;
                if (section2 == "sendsize") {
                    p_sendSize = pt2.get<uint32_t>(section2);
                }
                // reserved for back-compatible
                if (section2 == "maxbytes") {
                    p_maxBytes = pt2.get<uint32_t>(section2);
                }
                if (section2 == "application") {
                    const boost::property_tree::ptree &pt3 = it2.second;
                    AppConf app;
                    for (auto it3:pt3) {
                        auto section3 = it3.first;
                        if (section3 == "bytesList") {
                            app.bytesList = pt3.get<std::string>(section3);
                        }
                        if (section3 == "timeIntervalList") {
                            app.timeIntervalList = pt3.get<std::string>(section3);
                        }
                    }
                    p_apps.push_back(app);
                }
            }

            BOOST_ASSERT(p_sendSize != 0);
            ////BOOST_ASSERT(p_maxBytes != 0);
            //BOOST_ASSERT(!p_maxBytesList.empty());
            //BOOST_ASSERT(!p_timeIntervalList.empty());
        }
        else if (section == "Simulation") {
            p_simulationErrorRate = pt2.get<float>("errorRate");
        }
    }
}
