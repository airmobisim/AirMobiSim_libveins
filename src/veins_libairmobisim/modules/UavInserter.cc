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
        insertUavMessage = new cMessage("Insert UAV");
    } else if (stage == 1) {
        drociManager =  veins::FindModule<DroCIManager*>::findGlobalModule();
        scheduleAt(simTime() + 5, insertUavMessage);
    }
}

void UavInserter::handleMessage(cMessage *msg) {
    if(msg == insertUavMessage) {
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
                    << ", " << position.z << ")" << std::endl;
        }
        std::cout << "done!" << std::endl;
    }
}
