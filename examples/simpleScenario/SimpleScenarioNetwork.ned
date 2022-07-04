package veins_libairmobisim.examples.veins_libairmobisim;

import veins_libairmobisim.node.Uav;
import veins_libairmobisim.mobility.DroCIManager;

import org.car2x.veins.modules.world.annotations.AnnotationManager;
import org.car2x.veins.base.modules.BaseWorldUtility;
import org.car2x.veins.modules.obstacle.ObstacleControl;
import org.car2x.veins.base.connectionManager.ConnectionManager;

import org.car2x.veins.nodes.Car;
import org.car2x.veins.visualizer.roads.RoadsCanvasVisualizer;
import org.car2x.veins.modules.mobility.traci.TraCIScenarioManager*;

network SimpleScenarioNetwork
{
    parameters:
        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
    submodules:
  		obstacles: ObstacleControl {
            @display("p=240,50");
        }
        annotations: AnnotationManager {
            @display("p=260,50");
        }
        connectionManager: ConnectionManager {
            parameters:
                @display("p=150,0;i=abstract/multicast");
        }
        world: BaseWorldUtility {
            parameters:
                playgroundSizeX = veins_eval_by_version(veins_omnetpp_buildnum(), "playgroundSizeX", 1525, "parent.playgroundSizeX");
                playgroundSizeY = veins_eval_by_version(veins_omnetpp_buildnum(), "playgroundSizeY", 1525, "parent.playgroundSizeY");
                playgroundSizeZ = veins_eval_by_version(veins_omnetpp_buildnum(), "playgroundSizeZ", 1525, "parent.playgroundSizeZ");
                @display("p=2000,2000;i=misc/globe");
        }
		dManager: DroCIManager {
            @display("p=50,138");
        }
		manager: TraCIScenarioManagerForker {
            parameters:
                @display("p=512,128");
        }
        roadsCanvasVisualizer: RoadsCanvasVisualizer {
            @display("p=300,0");
        }
        uav[0]: Uav{
        }
        
    connections allowunconnected:
}
