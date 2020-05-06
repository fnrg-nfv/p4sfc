InfiniteSource( DATA \< 
// 00 00 00 03 00 20 01 70 0C DD
00 00 00 00 00 00 00 00 00 00 00 00 08 00 
45 00 00 2E 00 00 40 00 40 11 96 24 0A 00
00 01 4D 4D 4D 4D 22 B8 5B 25 00 1A DD 41
00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00>, LIMIT 100000000, STOP true) 
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
