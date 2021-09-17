package veins_libairmobisim2.examples.veins_libairmobisim;

import veins_libairmobisim.DroCIManager;


network SimpleScenarioNetwork
{
    @display("bgb=189,218");
    submodules:
        tic: DroCIManager {
            @display("p=50,138");
        }
        toc: DroCIManager {
            @display("p=145,22");
        }
    connections:
        tic.out --> {  delay = 0ms; } --> toc.in;
        tic.in <-- {  delay = 0ms; } <-- toc.out;
}
