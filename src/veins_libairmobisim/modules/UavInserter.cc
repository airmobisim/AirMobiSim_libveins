//
// Copyright (C) 2022 Tobias Hardes <tobias.hardes@upb.de>
// Copyright (C) 2022 Dalisha Logan <dalisha@mail.uni-paderborn.de>
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

#include "UavInserter.h"

using namespace airmobisim;
Define_Module(airmobisim::UavInserter);

void UavInserter::initialize(int stage){
    if (stage == 0) {
        queryDataMsg = new cMessage("Query UAV data");
        addWaypointMsg = new cMessage("Add new waypoint");
        insertNewUavMsg = new cMessage("Insert new UAV");
        deleteNewUavMsg = new cMessage("Delete UAV");
    } else if (stage == 1) {
        drociManager =  veins::FindModule<DroCIManager*>::findGlobalModule();
        scheduleAt(simTime() + 2, queryDataMsg);
        scheduleAt(simTime() + 2.2, addWaypointMsg);
        scheduleAt(simTime() + 5, insertNewUavMsg);
        scheduleAt(simTime() + 5.5, deleteNewUavMsg);

    }
}

void UavInserter::handleMessage(cMessage *msg) {
    if(msg == queryDataMsg) {
        auto count = drociManager->getCurrentUAVCount();
        std::cout << "Got " <<count <<  " UAVs in simulation"<< std::endl;
        airmobisim::UavList listOfUavs = drociManager->getManagedHosts();
        for (uint32_t i = 0; i < listOfUavs.uavs_size(); i++) {
            Coord position;
            double angle;
            position.x = listOfUavs.uavs(i).x();
            position.y = listOfUavs.uavs(i).y();
            position.z = listOfUavs.uavs(i).z();
            angle = listOfUavs.uavs(i).angle();
            std::cout << "have a UAV: ID " << listOfUavs.uavs(i).id()
                    << " at (x,y,z) (" << position.x << ", " << position.y
                    << ", " << position.z << ") - angle is " << angle << std::endl;
        }
        std::cout << "done!" << std::endl;
    } else if (msg == addWaypointMsg) {
        drociManager->insertWaypoint(drociManager->getMaxUavId(), 750, 750, 3);
    } else if (msg == insertNewUavMsg){
        auto insertUavId = drociManager->getMaxUavId() + 1;
        Coord startPosition;
        startPosition.x = 0;
        startPosition.y = 10;
        startPosition.z = 5;
        Coord endPosition;
        endPosition.x = 1000;
        endPosition.y = 10;
        endPosition.z = 5;
        auto startAngle = 90;
        auto speed = 20;
        std::cout << "Add new UAV with: ID " << insertUavId << std::endl;
        drociManager->insertUAV(insertUavId, startPosition, endPosition, startAngle, speed);

        uavIdToDelete = insertUavId;

    } else if (msg == deleteNewUavMsg) {
        drociManager->deleteUAV(uavIdToDelete);
    }
}
