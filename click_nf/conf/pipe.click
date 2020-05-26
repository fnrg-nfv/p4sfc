define($dev eth0)

FromDevice($dev) -> Print(in&out)
                 -> Queue(1024)
                 -> ToDevice($dev);

