FromDevice(veth1)
    -> c :: Counter
    -> Discard;

Script( TYPE ACTIVE,
    read c.rate,
    read c.bit_rate,
    read c.byte_rate,
    wait 1,
    loop,
    end,
    )
