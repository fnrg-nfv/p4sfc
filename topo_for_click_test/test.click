require(package "p4sfc");

ec :: P4SFCEncap();

src :: FromDevice(eth0);

src -> Print(src)
    -> [0]ec;

ec[0] -> Print(ec0)
      -> [1]ec;

ec[1] -> Print(ec1)
      -> Queue(1024)
      -> ToDevice(eth0);
