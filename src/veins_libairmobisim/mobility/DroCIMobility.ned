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

package veins_libairmobisim.mobility;

import org.car2x.veins.base.modules.BaseMobility;

simple DroCIMobility extends BaseMobility {
    parameters:
    	@class(airmobisim::DroCIMobility);
        double hostPositionOffset @unit("m") = default(0.0m);  // shift OMNeT++ module this far from front of the car
        bool setHostSpeed = default(false);  // whether to update the speed of the host (along with its position)        
}
