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
    void updateModulePosition(cModule* mod, const Coord& p, double speed); // TODO: Include heading here
    void addModule(std::string nodeId, std::string type, std::string name, std::string displayString, const Coord& position, double speed, double length, double height, double width); // TODO: Include heading here
    int numInitStages() const override
    {
        return std::max(cSimpleModule::numInitStages(), 2);
    }
private:
    void launchSimulator();
    void executeOneTimestep();
    void preInitializeModule(cModule* mod, const std::string& nodeId, const Coord& position, double speed);
    void processUavSubscription(std::string id, Coord p, double speed);
    cModule* getManagedModule(std::string nodeId);
    void insertUAV(Coord startPosition, Coord endPosition, double startAngle, double speed);
    void insertWaypoint();
    int getnumberCurrentUAV();
private:
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<airmobisim::AirMobiSim::Stub> stub;
    double updateInterval;
    double simTimeLimit;
    int count;
    double totalsteps;
    omnetpp::cMessage *initMsg;
    omnetpp::cMessage *executeOneTimestepTrigger;
    std::string moduleType;
    size_t nextNodeVectorIndex; /**< next OMNeT++ module vector index to use */
    std::map<std::string, cModule*> hosts; /**< vector of all hosts managed by us */

};


}



