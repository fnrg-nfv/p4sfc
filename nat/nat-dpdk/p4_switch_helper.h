#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <unistd.h>

#include "../../utils/p4runtime_cpp_lib/libswitch.h"
#include "nat_common.h"


general_switch_t switch_connect(char* grpc_addr, char* config_path, char* p4info_path, int dev_id);
void add_nat_rule(general_switch_t s, struct nat_rule *rule, uint32_t static_ip);