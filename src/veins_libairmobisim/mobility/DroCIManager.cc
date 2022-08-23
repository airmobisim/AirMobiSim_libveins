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

#include "DroCIManager.h"
#include <google/protobuf/empty.pb.h>
#include <grpcpp/grpcpp.h>

#include <string.h>
#include <unistd.h>
#include <memory.h>
#include <array>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <sys/wait.h>
#include <fstream>
using std::ifstream;
#include <sys/stat.h>

#include <vector>
#include <stdio.h>
#include <chrono>
#include <thread>
#include "unistd.h"

using namespace omnetpp;
using namespace airmobisim;

Define_Module(DroCIManager);

void DroCIManager::initialize(int stage) {
    if (stage == 0) {
        updateInterval = par("updateInterval").doubleValue();
        moduleType = par("moduleType").stringValue();
        moduleName = par("moduleName").stringValue();
        moduleDisplayString = par("moduleDisplayString").stdstringValue();

        simTimeLimit =
                std::stod(
                        std::string(
                                this->getSimulation()->getActiveSimulation()->getEnvir()->getConfig()->getConfigValue(
                                        "sim-time-limit")));
        totalsteps = simTimeLimit / updateInterval;

        // Do not create children here since OMNeT++ will try to initialize them again
        initMsg = new cMessage("init");
        launchSimulatorMsg = new cMessage();
        checkConnectionMsg = new cMessage();
        checkConnectionMsg->setKind(checkConnectionMsgKind);

        simulationStepCount = 0;
        nextNodeVectorIndex = 0;
        maxSimulationTimeAirMobiSim = 0;
        maxSimulationStepsAirMobiSim = 0.0;
        hosts.clear();
        executeOneTimestepTrigger = new cMessage("step");
    } else if (stage == 1) {
        startAirMobiSim();
    }
}

void DroCIManager::handleMessage(cMessage *msg) {
    if (msg == initMsg) {
        startAirMobiSim();
        return;
    }
    if (msg == launchSimulatorMsg) {
        launchSimulator();
        return;
    }
    if (msg->getKind() == checkConnectionMsgKind) {
        if (pid) {
            int status = 0;
            pid_t r;
            r = waitpid(pid, &status, WNOHANG);
            EV_DEBUG << "pid == " << pid << " - waitpid returned with " << r
                            << std::endl;
            if (r == 0) {
                EV_DEBUG << "no state change" << std::endl;
            } else if (r == -1) {
                EV_DEBUG << "no such pid" << std::endl;
            } else {
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    EV_DEBUG << "child died - goodbye" << std::endl;
                    endSimulation();
                }
            }
            scheduleAt(simTime() + updateInterval, checkConnectionMsg);
        }
        return;
    }
    if (msg == executeOneTimestepTrigger) {
        executeOneTimestep();
        return;
    }
    throw cRuntimeError("DroCIManager received unknown self-message");

}
void DroCIManager::startAirMobiSim() {
    pid = fork();

    if (pid == 0) {
        signal(SIGINT, SIG_IGN);

        int r = execl("/bin/sh", "sh", "-c",
                "cd $AIRMOBISIMHOME && poetry run ./airmobisim.py --omnetpp",
                NULL);

        if (r == -1) {
            throw cRuntimeError("system failed");
        }
        if (WEXITSTATUS(r) != 0) {
            throw cRuntimeError("cannot run");
        }
        throw cRuntimeError("returned from exec");
        exit(1);
    }
    scheduleAt(simTime() + updateInterval, launchSimulatorMsg);
    scheduleAt(simTime() + updateInterval, checkConnectionMsg);
}

airmobisim::UavList DroCIManager::getManagedHosts() {
    grpc::ClientContext clientContext;
    airmobisim::UavList managedHosts;
    google::protobuf::Empty empty;
    grpc::Status status = stub->GetManagedHosts(&clientContext, empty,
            &managedHosts);
    if (status.ok()) {
        return managedHosts;
    } else {
        error(
                (std::string(
                        "DroCIManager::getManagedHosts() failed with error: "
                                + std::string(status.error_message())).c_str()));
    }
}

void DroCIManager::launchSimulator() {
    std::array<char, 128> buffer;
    std::string pathToAirMobiSim;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(
            popen("cd $AIRMOBISIMHOME && pwd", "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        pathToAirMobiSim += buffer.data();
    }
    pathToAirMobiSim.pop_back(); // remove line break
    ifstream indata;
    int portNumber;
    std::string filepath = pathToAirMobiSim + "/" + std::to_string(pid)
            + ".tmp";
    struct stat buffer2;
    while (!stat(filepath.c_str(), &buffer2) == 0) {

    }
    indata.open(filepath); // opens the file
    if (!indata) { // file couldn't be opened
        std::cerr << "Error: port number file could not be opened:" << filepath
                << std::endl;
        exit(1);
    }
    indata >> portNumber;
    indata.close();
    std::remove(filepath.c_str());
    std::cout << "Port of AirMobiSim is " << portNumber << std::endl;
    std::string hostAndPort = "localhost:" + std::to_string(portNumber);

    channel = CreateChannel(hostAndPort, grpc::InsecureChannelCredentials());
    stub = airmobisim::AirMobiSim::NewStub(channel);
    auto state = channel->GetState(true);

    while (state != GRPC_CHANNEL_READY) {
        if (!channel->WaitForStateChange(state,
                std::chrono::system_clock::now() + std::chrono::seconds(15))) {
            error("Could not connect to gRPC");
        }
        state = channel->GetState(true);
    }

    airmobisim::UavList managedHosts;
    google::protobuf::Empty empty;
    grpc::ClientContext clientContext;
    google::protobuf::Empty empty2;
    grpc::ClientContext clientContext2;
    google::protobuf::Empty empty3;
    grpc::ClientContext clientContext3;
    airmobisim::Number airMobiSimSimulationTimeLimit;
    airmobisim::DoubleNumber airMobiSimSimulationSteps;

    grpc::Status status = stub->GetMaxSimulationTime(&clientContext, empty,
            &airMobiSimSimulationTimeLimit);
    if (status.ok()) {
        maxSimulationTimeAirMobiSim = airMobiSimSimulationTimeLimit.num();
    } else {
        error(
                (std::string(
                        "DroCIManager::GetMaxSimulationTime() failed with error: "
                                + std::string(status.error_message())).c_str()));
    }

    status = stub->getMaxSimulationSteps(&clientContext2, empty2,
            &airMobiSimSimulationSteps);
    if (status.ok()) {
        maxSimulationStepsAirMobiSim = airMobiSimSimulationTimeLimit.num();
    } else {
        error(
                (std::string(
                        "DroCIManager::getMaxSimulationSteps() failed with error: "
                                + std::string(status.error_message())).c_str()));
    }

    status = stub->GetManagedHosts(&clientContext3, empty3, &managedHosts);

    std::cout << simTime().dbl() << ": launchSimulator()" << std::endl;
    if (status.ok()) {
        for (uint32_t i = 0; i < managedHosts.uavs_size(); i++) {
            Coord position;
            double angle;
            position.x = managedHosts.uavs(i).x();
            position.y = managedHosts.uavs(i).y();
            position.z = managedHosts.uavs(i).z();
            angle = managedHosts.uavs(i).angle();
            std::cout << "Add module " << managedHosts.uavs(i).id()
                    << " at (x,y,z) (" << position.x << ", " << position.y
                    << ", " << position.z << ")" << std::endl;

            auto speed = 0;
            auto length = 2;
            auto height = 2;
            auto width = 2;
            auto mName = "uav";
            std::stringstream ss;
            ss << managedHosts.uavs(i).id();
            std::string id = ss.str();
            addModule(id, moduleType, mName, moduleDisplayString, position,
                    speed, angle, length, height, width);
        }
    } else {
        error(
                (std::string(
                        "DroCIManager::GetManagedHosts() failed with error: "
                                + std::string(status.error_message())).c_str()));
    }
    if (simulationStepCount < totalsteps) {
        scheduleAt(simTime() + updateInterval, executeOneTimestepTrigger);
    } else {
        EV << "End" << endl;
    }
}

void DroCIManager::executeOneTimestep() {
    EV_DEBUG << "Triggering AirMobiSim simulator advance to t=" << simTime()
                    << endl;
    airmobisim::ResponseQuery response;
    google::protobuf::Empty empty;
    grpc::ClientContext clientContext;
    grpc::Status status = stub->ExecuteOneTimeStep(&clientContext, empty,
            &response);

    if (status.ok()) {
        for (uint32_t i = 0; i < response.responses_size(); i++) {
            EV << "Length of response" << response.responses_size() << endl;
            EV << "Getting for " << response.responses(i).id()
                      << " subscription results" << endl;
            EV << "Position" << response.responses(i).y() << endl;

            std::stringstream ss;
            ss << response.responses(i).id();
            processUavSubscription(ss.str(),
                    Coord(response.responses(i).x(), response.responses(i).y(),
                            response.responses(i).z()),
                    response.responses(i).speed(),
                    response.responses(i).angle());
        }
    } else {
        if (simulationStepCount > maxSimulationStepsAirMobiSim) { // There are no further timesteps in AirMobiSim
            endSimulation();
        } else {
            error(
                    (std::string(
                            "DroCIManager::executeOneTimestep() failed with error: "
                                    + std::string(status.error_message())).c_str()));
        }
    }

    simulationStepCount = simulationStepCount + 1;

    if (simulationStepCount < totalsteps) {
        scheduleAt(simTime() + updateInterval, executeOneTimestepTrigger);
    } else {
        EV << "End" << endl;
    }

}

void DroCIManager::processUavSubscription(std::string objectId, Coord p,
        double speed, double angle) {
    cModule *mod = getManagedModule(objectId);

    if (!mod) {
        error("DroCIManager::processUavSubscription: No such module found");
    } else {
        // module existed - update position
        EV_DEBUG << "module " << objectId << " moving to " << p.x << "," << p.y
                        << "with heading" << angle << endl;
        updateModulePosition(mod, p, speed, angle);
    }
}

cModule* DroCIManager::getManagedModule(std::string nodeId) {
    if (hosts.find(nodeId) == hosts.end())
        return nullptr;
    return hosts[nodeId];
}

void DroCIManager::addModule(std::string nodeId, std::string moduleType,
        std::string moduleName, std::string displayString,
        const Coord &position, double speed, double angle, double length,
        double height, double width) {

    std::cout << "addModule called for module id " << nodeId << std::endl;
    int32_t nodeVectorIndex = nextNodeVectorIndex++;

    cModule *parentmod = getParentModule();
    if (!parentmod)
        throw cRuntimeError("Parent Module not found");

    cModuleType *nodeType = cModuleType::get(moduleType.c_str());
    if (!nodeType)
        throw cRuntimeError("Module Type \"%s\" not found", moduleType.c_str());

#if OMNETPP_BUILDNUM >= 1525
    parentmod->setSubmoduleVectorSize(moduleName.c_str(), nodeVectorIndex + 1);
    cModule* mod = nodeType->create(moduleName.c_str(), parentmod, nodeVectorIndex);
#else
    cModule *mod = nodeType->create(moduleName.c_str(), parentmod,
            nodeVectorIndex, nodeVectorIndex);
#endif
    mod->finalizeParameters();
    if (displayString.length() > 0) {
        mod->getDisplayString().parse(displayString.c_str());
    }
    mod->buildInside();
    mod->scheduleStart(simTime() + updateInterval);

    preInitializeModule(mod, nodeId, position, speed, angle);

    mod->callInitialize();
    hosts[nodeId] = mod;

}

void DroCIManager::preInitializeModule(cModule *mod, const std::string &nodeId,
        const Coord &position, double speed, double angle) {
    // pre-initialize Mobility
    auto mobilityModules = getSubmodulesOfType<DroCIMobility>(mod);
    for (auto mm : mobilityModules) {
        mm->preInitialize(nodeId, position, speed, angle);
    }
}

void DroCIManager::updateModulePosition(cModule *mod, const Coord &p,
        double speed, double angle) {
    // update position in DroCIMobility
    auto mobilityModules = getSubmodulesOfType<DroCIMobility>(mod);
    for (auto mm : mobilityModules) {
        mm->nextPosition(p, speed, angle);
    }
}

int DroCIManager::getCurrentUAVCount() {
    airmobisim::Number number_uav;
    google::protobuf::Empty empty;
    grpc::ClientContext clientcontext;
    grpc::Status status;

    if (stub != nullptr) {
        status = stub->getNumberCurrentUAV(&clientcontext, empty, &number_uav);
    }
    if (!status.ok()) {
        error(
                (std::string(
                        "DroCIManager::getnumberCurrentUAV() failed with error: "
                                + std::string(status.error_message())).c_str()));
    }
    return number_uav.num();
}

void DroCIManager::deleteUAV(int deleteUavId) {
    EV << "This is my deleteUavId" << deleteUavId << endl;

    airmobisim::Number nodeId;
    google::protobuf::Empty empty;
    grpc::ClientContext clientcontext;

    nodeId.set_num(deleteUavId);

    grpc::Status status = stub->DeleteUAV(&clientcontext, nodeId, &empty);
    if (!status.ok()) {
        error(
                (std::string(
                        "DroCIManager::deleteUAV() failed with error: "
                                + std::string(status.error_message())).c_str()));
    }
    cModule *node = getManagedModule(std::to_string(deleteUavId));
    node->deleteModule();

    auto it = hosts.find(std::to_string(deleteUavId));
    hosts.erase(it);
}

void DroCIManager::insertUAV(int insertUavId, Coord startPosition,
        Coord endPosition, double startAngle, double speed) {

    airmobisim::StartUav *startuav = new StartUav;

    google::protobuf::Empty empty;
    grpc::ClientContext clientcontext;

    airmobisim::Coordinates *startpos = startuav->add_coordinates();
    airmobisim::Coordinates *endpos = startuav->add_coordinates();

    //Setting the Coordinates of the Startposition
    startpos->set_x(startPosition.x);
    startpos->set_y(startPosition.y);
    startpos->set_z(startPosition.z);

    //Setting the Coordinates of the Endposition
    endpos->set_x(endPosition.x);
    endpos->set_y(endPosition.y);
    endpos->set_z(endPosition.z);

    startuav->set_id(insertUavId);
    startuav->set_angle(startAngle);
    startuav->set_speed(speed);

    grpc::Status status = stub->InsertUAV(&clientcontext, *startuav, &empty);

    if (!status.ok()) {
        error("DroCIManager::insertUAV() has failed!");
    }
    auto width = 1.0;
    auto length = 1.0;

    addModule(std::to_string(insertUavId), moduleType.c_str(),
            moduleName.c_str(), moduleDisplayString, startPosition, speed,
            startAngle, length, startPosition.z, width);
}

int DroCIManager::getMaxUavId() {
    grpc::ClientContext clientContext;
    airmobisim::Number maxUavId;
    google::protobuf::Empty empty;
    grpc::Status status = stub->getMaxUavId(&clientContext, empty, &maxUavId);
    if (status.ok()) {
        return maxUavId.num();
    } else {
        error(
                (std::string(
                        "DroCIManager::getMaxUavId() failed with error: "
                                + std::string(status.error_message())).c_str()));
    }
}

void DroCIManager::insertWaypoint(int uavId, double x, double y, double z,
        int index) {
    grpc::ClientContext clientContext;
    google::protobuf::Empty empty;

    airmobisim::Waypoint *waypoint = new Waypoint();
    std::cout << "UAV-ID is " << uavId << std::endl;
    waypoint->set_index(index);
    waypoint->set_x(x);
    waypoint->set_y(y);
    waypoint->set_z(z);
    waypoint->set_uid(uavId);

    grpc::Status status = stub->InsertWaypoint(&clientContext, *waypoint,
            &empty);

    if (!status.ok()) {
        error(
                (std::string(
                        "DroCIManager::insertWaypoint() failed with error: "
                                + std::string(status.error_message())).c_str()));
    }
}

void DroCIManager::setDesiredSpeed() {
    EV << "setDesiredSpeed is called" << endl;
    airmobisim::UavSetSpeed *uavsetspeed = new UavSetSpeed;
    grpc::ClientContext clientcontext;

    google::protobuf::Empty empty;

    uavsetspeed->set_id(0);
    uavsetspeed->set_speed(40);

    grpc::Status status = stub->SetDesiredSpeed(&clientcontext, *uavsetspeed,
            &empty);

    if (!status.ok()) {
        error(
                (std::string(
                        "DroCIManager::setDesiredSpeed() failed with error: "
                                + std::string(status.error_message())).c_str()));
    }
}

void DroCIManager::updateWaypoints() {

    EV << "updateWaypoints is called" << endl;

    airmobisim::WaypointList *waypointlist = new WaypointList;

    grpc::ClientContext clientContext;
    google::protobuf::Empty empty;

    //TODO:Needs to be change! Add a for loop!
    airmobisim::Waypoint *waypoint1 = waypointlist->add_waypoint();
    airmobisim::Waypoint *waypoint2 = waypointlist->add_waypoint();

    waypointlist->set_id(0);

    // Needs to be changed in the future, these are just example values.
    waypoint1->set_index(0);
    waypoint1->set_x(6.5);
    waypoint1->set_y(10);
    waypoint1->set_z(3);

    waypoint2->set_index(1);
    waypoint2->set_x(6.8);
    waypoint2->set_y(10);
    waypoint2->set_z(3);

    grpc::Status status = stub->UpdateWaypoints(&clientContext, *waypointlist,
            &empty);

    if (!status.ok()) {
        error(
                (std::string(
                        "DroCIManager::updateWaypoints() failed with error: "
                                + std::string(status.error_message())).c_str()));
    }
}
