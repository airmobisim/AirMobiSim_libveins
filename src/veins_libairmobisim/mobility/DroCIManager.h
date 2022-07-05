//
// Copyright (C) 2022 Tobias Hardes <tobias.hardes@upb.de>
// Copyright (C) 2022 Dalisha Logan <dalisha@mail.uni-paderborn.de>
// Copyright (C) 2022 Christoph Sommer <sommer@cms-labs.org>
//
// Documentation for these modules is at http://veins.car2x.org/
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
#pragma once

#include <map>
#include <memory>

#include <omnetpp.h>
#include <cstdlib>
#include <iostream>
#include <regex>
#include <sstream>

#include "veins/base/utils/Coord.h"
#include "veins/base/utils/FindModule.h"

#include <google/protobuf/empty.pb.h>
#include <grpcpp/grpcpp.h>


#include "DroCIMobility.h"
#include "../proto/airmobisim.grpc.pb.h"

#include "veins_libairmobisim/veins_libairmobisim.h"
using namespace veins;

namespace airmobisim {
class DroCIManager : public omnetpp::cSimpleModule
{
protected:
    void initialize(int stage) override;
    void handleMessage(omnetpp::cMessage *msg) override;
    void updateModulePosition(cModule* mod, const Coord& p, double speed, double angle);
    void addModule(std::string nodeId, std::string type, std::string name, std::string displayString, const Coord& position, double speed, double angle, double length, double height, double width);
    int numInitStages() const override
    {
        return std::max(cSimpleModule::numInitStages(), 2);
    }
private:
    void launchSimulator();
    void executeOneTimestep();
    void preInitializeModule(cModule* mod, const std::string& nodeId, const Coord& position, double speed, double angle);
    void processUavSubscription(std::string id, Coord p, double speed, double angle);

public:
    void insertUAV(int insertUavId, Coord startPosition, Coord endPosition, double startAngle, double speed);
    void insertWaypoint(double x, double y, double z, int index = 0);

    void updateWaypoints();

    void deleteUAV(int deleteUavId);

    void setDesiredSpeed();

    int getCurrentUAVCount();
    cModule* getManagedModule(std::string nodeId);

private:
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<airmobisim::AirMobiSim::Stub> stub;
    double updateInterval;
    double simTimeLimit;
    int count;
    double totalsteps;
    omnetpp::cMessage *initMsg;
    omnetpp::cMessage *launchSimulatorMsg;
    omnetpp::cMessage *executeOneTimestepTrigger;
    omnetpp::cMessage *checkConnectionMsg;
    int checkConnectionMsgKind = 1337;
    std::string moduleType;
    size_t nextNodeVectorIndex; /**< next OMNeT++ module vector index to use */
    std::map<std::string, cModule*> hosts; /**< vector of all hosts managed by us */

protected:
    void startAirMobiSim();
    pid_t pid;
};


}



