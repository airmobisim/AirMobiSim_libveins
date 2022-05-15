//
// Copyright (C) 2021 Tobias Hardes <tobias.hardes@upb.de>
// Copyright (C) 2021 Dalisha Logan
// Copyright (C) 2006-2017 Christoph Sommer <sommer@ccs-labs.org>
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


#include <vector>
#include <stdio.h>
#include <chrono>
#include <thread>
#include "unistd.h"


using namespace omnetpp;
using namespace airmobisim;

Define_Module(DroCIManager);

void DroCIManager::initialize(int stage)
{
    if (stage == 0) {
        updateInterval = par("updateInterval").doubleValue();
        simTimeLimit = 55;//par(""); //TODO
        moduleType = par("moduleType").stringValue();

        totalsteps = simTimeLimit / updateInterval;

        // Do not create children here since OMNeT++ will try to initialize them again
        initMsg = new cMessage("init");
        launchSimulatorMsg = new cMessage();
        checkConnectionMsg = new cMessage();
        checkConnectionMsg->setKind(checkConnectionMsgKind);

        count = 0;
        nextNodeVectorIndex = 0;

        hosts.clear();
        executeOneTimestepTrigger = new cMessage("step");
    } else if (stage == 1) {
        scheduleAt(simTime() + updateInterval, initMsg);
    }
}

void DroCIManager::handleMessage(cMessage* msg)
{
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
            EV_DEBUG << "pid == " << pid << " - waitpid returned with " << r << std::endl;
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
            scheduleAt(simTime()+updateInterval, checkConnectionMsg);
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

    std::cout << "OMNeT++ my pID is " << getpid() << std::endl;
    std::cout << "spawn" << std::endl;

    pid = fork();
    std::cout << "pid is " << pid << std::endl;

    if (pid == 0) {
        signal(SIGINT, SIG_IGN);

        //int r = execl("/bin/sh", "sh", "-c", "./launchAirMobiSim.sh", NULL);
        int r = execl("/bin/sh", "sh", "-c", "airmobisim.py --omnetpp", NULL); // This is the PID we check

       //int r = execl("/bin/sh", "sh", "-c", "./abel.py", NULL);

        std::cout << "execl done" << pid << std::endl;
        if (r == -1) {
            throw cRuntimeError("system failed");
        }
        if (WEXITSTATUS(r) != 0) {
            throw cRuntimeError("cannot run");

        }
        throw cRuntimeError("returned from exec");
        exit(1);
    }

    sleep(5);
    scheduleAt(simTime() + updateInterval, launchSimulatorMsg);
    scheduleAt(simTime() + updateInterval, checkConnectionMsg);
}




void DroCIManager::launchSimulator() {


    channel = CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
    stub = airmobisim::AirMobiSim::NewStub(channel);

    int number = 0;

    airmobisim::UavList managedHosts;
    google::protobuf::Empty empty;
    grpc::ClientContext clientContext;
    grpc::Status status = stub->GetManagedHosts(&clientContext, empty, &managedHosts);

    std::cout << simTime().dbl() << ": launchSimulator()" << std::endl;
    if (status.ok()) {
        for (uint32_t i = 0; i < managedHosts.uavs_size(); i++) {
            Coord position;
            position.x = managedHosts.uavs(i).x();
            position.y = managedHosts.uavs(i).y();
            position.z = managedHosts.uavs(i).z();
            std::cout << "Add module "<< managedHosts.uavs(i).id() <<" at (x,y,z) (" << position.x << ", " << position.y << ", "<< position.z << ")" << std::endl;
            auto dString = "i=airmobisim/uav/uav;is=vs";
            auto speed  = 0;
            auto length = 2;
            auto height = 2;
            auto width  = 2;
            auto mName = "uav";
            std::stringstream ss;
            ss << managedHosts.uavs(i).id();
            std::string id = ss.str();
            addModule(id, moduleType, mName, dString, position, speed, length, height, width);
        }
    } else {
        std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
    std::cout << "done" << std::endl;


    if (count < totalsteps) {
        scheduleAt(simTime() + updateInterval, executeOneTimestepTrigger);
    } else {
        EV << "End" << endl;
    }

   //insertUAV(Coord(1000,1000,3), Coord(0,5,7), 20.0, 10.0);

    //number = getnumberCurrentUAV();
    //EV << "This is the number of UAVs:" << number << endl;

}

void DroCIManager::executeOneTimestep()
{
    EV_DEBUG << "Triggering AirMobiSim simulator advance to t=" << simTime() << endl;

    int number = 0;

    airmobisim::ResponseQuery response;
    google::protobuf::Empty empty;
    grpc::ClientContext clientContext;
    grpc::Status status = stub->ExecuteOneTimeStep(&clientContext, empty, &response);

    if (status.ok()) {
        for (uint32_t i = 0; i < response.responses_size(); i++) {
            EV << "Length of response" << response.responses_size() << endl;
            EV << "Getting for " << response.responses(i).id()
                      << " subscription results" << endl;
            EV << "Position" << response.responses(i).y() << endl;


            std::stringstream ss;
            ss << response.responses(i).id();
            processUavSubscription(ss.str(), Coord(response.responses(i).x(), response.responses(i).y(), response.responses(i).z()),response.responses(i).speed());
        }
    } else {
        std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
    }
    
    count = count + 1;

    if(count < totalsteps) {
        scheduleAt(simTime() + updateInterval, executeOneTimestepTrigger);
    } else {
        EV << "End" << endl;
    }

}

void DroCIManager::processUavSubscription(std::string objectId, Coord p, double speed) {
    cModule* mod = getManagedModule(objectId);

    if (!mod) {
        // TODO: Add a new model
    } else {
        // module existed - update position
        EV_DEBUG << "module " << objectId << " moving to " << p.x << "," << p.y << endl;
        updateModulePosition(mod, p, speed);
    }
}

cModule* DroCIManager::getManagedModule(std::string nodeId) {
    if (hosts.find(nodeId) == hosts.end()) return nullptr;
    return hosts[nodeId];
}

void DroCIManager::addModule(std::string nodeId, std::string type, std::string name, std::string displayString, const Coord& position, double speed, double length, double height, double width) {

    std::cout << "addModule called for module id " << nodeId << std::endl;
    int32_t nodeVectorIndex = nextNodeVectorIndex++;

    cModule* parentmod = getParentModule();
    if (!parentmod) throw cRuntimeError("Parent Module not found");

    cModuleType* nodeType = cModuleType::get(type.c_str());
    if (!nodeType) throw cRuntimeError("Module Type \"%s\" not found", type.c_str());

    // TODO: this trashes the vectsize member of the cModule, although nobody seems to use it
    cModule* mod = nodeType->create(name.c_str(), parentmod, nodeVectorIndex, nodeVectorIndex);

    mod->finalizeParameters();
    if (displayString.length() > 0) {
        mod->getDisplayString().parse(displayString.c_str());
    }
    mod->buildInside();
    mod->scheduleStart(simTime() + updateInterval);

    preInitializeModule(mod, nodeId, position, speed);

    mod->callInitialize();
    hosts[nodeId] = mod;

    // post-initialize DroCIMobility
    auto mobilityModules = getSubmodulesOfType<DroCIMobility>(mod);
    for (auto mm : mobilityModules) {
        mm->changePosition();
    }
}

void DroCIManager::preInitializeModule(cModule* mod, const std::string& nodeId, const Coord& position, double speed)
{
    // pre-initialize Mobility
    auto mobilityModules = getSubmodulesOfType<DroCIMobility>(mod);
    for (auto mm : mobilityModules) {
        mm->preInitialize(nodeId, position, speed);
    }
}

void DroCIManager::updateModulePosition(cModule* mod, const Coord& p, double speed) {
    // update position in TraCIMobility
    auto mobilityModules = getSubmodulesOfType<DroCIMobility>(mod);
    for (auto mm : mobilityModules) {
        mm->nextPosition(p, speed);
    }
}


int DroCIManager::getnumberCurrentUAV(){

  //EV <<"getNumberCurrentUAV is getting called"<<endl;

  int currentUAV = 0;

  airmobisim::Number number_uav;
  google::protobuf::Empty empty;
  grpc::ClientContext clientcontext;

  grpc::Status status = stub->getNumberCurrentUAV(&clientcontext, empty, &number_uav);

  if (status.ok()){
      currentUAV = number_uav.num();

      EV << "I am OK" << endl;
  }

  else {

      EV << " I am not ok" << status.ok() << endl;
  }


  return currentUAV;
}




void DroCIManager::insertUAV(Coord startPosition, Coord endPosition, double startAngle, double speed){

    airmobisim::StartUav* startuav = new StartUav;

    google::protobuf::Empty empty;
    grpc::ClientContext clientcontext;


    airmobisim::Coordinates* startpos = startuav->add_coordinates();
    airmobisim::Coordinates* endpos = startuav->add_coordinates();

    //Setting the Coordinates of the Startposition
    startpos->set_x(startPosition.x);
    startpos->set_y(startPosition.y);
    startpos->set_z(startPosition.z);

    //Setting the Coordinates of the Endposition
    endpos->set_x(endPosition.x);
    endpos->set_y(endPosition.y);
    endpos->set_z(endPosition.z);

    startuav->set_angle(startAngle);
    startuav->set_speed(speed);



    grpc::Status status = stub->InsertUAV(&clientcontext, *startuav, &empty);



}



void DroCIManager::insertWaypoint(){

    airmobisim::WaypointList* waypointlist = new WaypointList;

    grpc::ClientContext clientContext;
    google::protobuf::Empty empty;

    //TODO:Needs to be change! Add a for loop!
    airmobisim::Waypoint* waypoint1 =  waypointlist->add_waypoint();
    airmobisim::Waypoint* waypoint2 =  waypointlist->add_waypoint();

    waypoint1->set_index(2);
    waypoint1->set_x(4.6);
    waypoint1->set_y(7.8);
    waypoint1->set_z(9.7);

    waypoint2->set_index(3);
    waypoint2->set_x(4.5);
    waypoint2->set_y(3.0);
    waypoint2->set_z(10.0);


    grpc::Status status = stub->InsertWaypoints(&clientContext, *waypointlist, &empty);
}

