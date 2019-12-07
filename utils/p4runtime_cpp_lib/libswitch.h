#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* general_switch_t;

general_switch_t GS_connect(char* grpc_addr, char* config_path, char* p4info_path);

void GS_run(general_switch_t self);

void GS_query_counter(general_switch_t self, char* counter_name, int index, uint64_t* packets, uint64_t* bytes);

#ifdef __cplusplus
}
#endif
