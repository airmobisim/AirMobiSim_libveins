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

#include "veins_libairmobisim/veins_libairmobisim.h"
#include "veins/base/utils/Coord.h"
#include "veins/base/modules/BaseMobility.h"
#include "veins/base/utils/FindModule.h"

using namespace omnetpp;
using namespace veins;

namespace airmobisim {

class DroCIMobility  : public BaseMobility {

  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;

  public:
    DroCIMobility():BaseMobility(),isPreInitialized(false) {

    }
    void preInitialize(std::string external_id, const Coord& position, double speed); //TODO: Include heading;
    virtual void changePosition();
    void nextPosition(const Coord& position, double speed);
  protected:
    cOutVector currentPosXVec; /**< vector plotting posx */
    cOutVector currentPosYVec; /**< vector plotting posy */
    cOutVector currentSpeedVec; /**< vector plotting speed */
    cOutVector currentAccelerationVec; /**< vector plotting acceleration */
    cOutVector currentCO2EmissionVec; /**< vector plotting current CO2 emission */

    bool isPreInitialized; /**< true if preInitialize() has been called immediately before initialize() */

    std::string external_id; /**< updated by setExternalId() */
    double hostPositionOffset; /**< front offset for the antenna on this car */
    bool setHostSpeed; /**< whether to update the speed of the host (along with its position)  */

    simtime_t lastUpdate; /**< updated by nextPosition() */
    Coord roadPosition; /**< position of front bumper, updated by nextPosition() */
    double speed; /**< updated by nextPosition() */

  private:

    /**
     * Calculates where the OMNeT++ module position of this UAV should be, given its front position
     */
    Coord calculateHostPosition(const Coord& vehiclePos) const;

    void fixIfHostGetsOutside() override; /**< called after each read to check for (and handle) invalid positions */
};


class VEINS_API DroCIMobilityAccess {
public:
    DroCIMobility* get(cModule* host)
    {
        DroCIMobility* droci = FindModule<DroCIMobility*>::findSubModule(host);
        ASSERT(droci);
        return droci;
    };
};
}
