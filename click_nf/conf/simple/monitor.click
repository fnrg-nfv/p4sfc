define($dev eth0)
require(package "p4sfc");

mn :: SampleMonitor();

FromDevice($dev) -> mn 
                 -> Discard;

