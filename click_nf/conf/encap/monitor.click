define($dev eth0);
require(package "p4sfc");
ec :: P4SFCEncap();

FromDevice($dev) -> [0]ec;
out :: Print(out)
    -> [1]ec;
ec[1] -> Queue(1024)
      -> ToDevice($dev);

mn :: SampleMonitor();

ec[0] -> mn 
      -> out;
