define($dev vEth0_1);
require(package "p4sfc");
ec :: P4SFCEncap();

FromDevice($dev)
	-> Strip(14)
	-> [0]ec;
out :: Counter
    -> [1]ec;
ec[1] 	-> EtherEncap(0x1234, 0:0:0:0:0:0, 0:0:0:0:0:0)
	-> Queue(1024)
      	-> ToDevice($dev);

mn :: SampleMonitor();

ec[0] -> mn 
      -> out;
