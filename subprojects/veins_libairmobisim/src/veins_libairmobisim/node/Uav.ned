//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package veins_libairmobisim.node;

import veins_libairmobisim.mobility.DroCIMobility;

module Uav
{
  parameters:
        string applType = default("ExampleAirMobiSimApp"); //type of the application layer
        string nicType  = default("Nic80211p"); // type of network interface card
        string drocimobilityType = default("DroCIMobility"); //type of the mobility module
    gates:
        input veinsradioIn;
    submodules:
        appl: <applType> like org.car2x.veins.base.modules.IBaseApplLayer {
            parameters:
                @display("p=60,50");
        }

        nic: <nicType> like org.car2x.veins.modules.nic.INic80211p {
          parameters:
               @display("p=60,166");
        }
        //drociMobility: <drocimobilityType> like org.car2x.veins.base.modules.IMobility {
        drociMobility: DroCIMobility {
            parameters:
                @display("p=130,172;i=block/cogwheel");
        }
        
     connections:
        nic.upperLayerOut --> appl.lowerLayerIn;
        nic.upperLayerIn <-- appl.lowerLayerOut;
        nic.upperControlOut --> appl.lowerControlIn;
        nic.upperControlIn <-- appl.lowerControlOut;
        veinsradioIn --> nic.radioIn;
}