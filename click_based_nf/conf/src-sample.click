
src :: InfiniteSource(
DATA \<
00 00 00 00 00 00 00 00 00 00 00 00 08 00 
45 00 00 2E 00 00 40 00 40 11 96 24 0A 00
00 01 4D 4D 4D 4D 22 B8 5B 25 00 1A DD 41
00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 00>,
LIMIT 1, STOP true);

// src :: FromDevice(veth1);
src -> Print(in&out)
    -> Queue(1024)
    -> ToDevice(veth1);