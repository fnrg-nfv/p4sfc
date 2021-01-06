define(
        $dev    0,
        $debug  false
        )

FromDPDKDevice($dev, PROMISC true) 
	-> Print(in&out, ACTIVE $debug)
        -> ToDPDKDevice($dev);

