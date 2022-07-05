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

simple DroCIManager {
    parameters:
    	@class(airmobisim::DroCIManager);
        double updateInterval @unit("s") = default(1s);  // time interval of hosts' position updates
        string moduleType = default("veins_libairmobisim.node.Uav");  // module type to be used in the simulation for each managed vehicle
        string host = default("localhost");  // server hostname
        double simTimeLimit @unit("s");
}