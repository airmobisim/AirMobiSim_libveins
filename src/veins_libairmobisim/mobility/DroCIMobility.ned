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


package veins_libairmobisim.mobility;

import org.car2x.veins.base.modules.BaseMobility;

simple DroCIMobility extends BaseMobility {
    parameters:
    	@class(airmobisim::DroCIMobility);
        double hostPositionOffset @unit("m") = default(0.0m);  // shift OMNeT++ module this far from front of the car
        bool setHostSpeed = default(false);  // whether to update the speed of the host (along with its position)        
}
