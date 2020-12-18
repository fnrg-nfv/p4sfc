define($dev vEth0_0);

FromDevice($dev)
    -> Queue(1024)
    -> Counter
    -> ToDevice($dev);