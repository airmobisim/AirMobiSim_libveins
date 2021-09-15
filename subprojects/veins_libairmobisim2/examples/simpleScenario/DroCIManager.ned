package veins_libairmobisim2.examples.veins_libairmobisim;



simple DroCIManager
{
    gates:
        input in;
        output out;
}

network DroCIManager1
{
    submodules:
        tic: DroCIManager;
        toc: DroCIManager;
    connections:
        tic.out --> {  delay = 0ms; } --> toc.in;
        tic.in <-- {  delay = 0ms; } <-- toc.out;
}