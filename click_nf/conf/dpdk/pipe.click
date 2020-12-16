define($dev 0)

FromDPDKDevice($dev, PROMISC true) -> Print(in&out)
                 -> ToDPDKDevice($dev);

