#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* general_switch_t;

struct GS_match_field_lpm_t {
  const char* name;
  const char* value;
  int size;
  int plen;
  struct GS_match_field_lpm_t *next;
};
typedef struct GS_match_field_lpm_t GS_match_field_lpm_t;

struct GS_match_field_exact_t {
  const char* name;
  const char* value;
  int size;
  struct GS_match_field_exact_t *next;
};
typedef struct GS_match_field_exact_t GS_match_field_exact_t;

struct GS_action_para_t {
  const char* name;
  const char* value;
  int size;
  struct GS_action_para_t *next;
};
typedef struct GS_action_para_t GS_action_para_t;

struct GS_table_entry_t {
  const char* table_name;
  const char* action_name;
  GS_match_field_lpm_t *match_field_lpm;
  GS_match_field_exact_t *match_field_exact;
  GS_action_para_t *action_para;
};
typedef struct GS_table_entry_t GS_table_entry_t;

general_switch_t GS_connect(const char* grpc_addr, const char* config_path, const char* p4info_path, int dev_id);

void GS_run(general_switch_t self);

void GS_query_counter(general_switch_t self, char* counter_name, int index, uint64_t* packets, uint64_t* bytes);

int GS_add_table_entry(general_switch_t untyped_self, GS_table_entry_t *table_entry);

#ifdef __cplusplus
}
#endif
