#include "libswitch.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <arpa/inet.h>

#include <ctype.h>
#include <unistd.h>

char *opt_config_path = NULL;
char *opt_p4info_path = NULL;

void print_help(const char *name) {
  fprintf(stderr,
          "Usage: %s [OPTIONS]...\n"
          "PI example controller app\n\n"
          "-c          P4 config (json)\n"
          "-p          P4Info (in protobuf text format);\n"
          "             if missing it will be generated from the config JSON\n",
          name);
}

int parse_opts(int argc, char *const argv[]) {
  int c;

  opterr = 0;

  while ((c = getopt(argc, argv, "c:p:h")) != -1) {
    switch (c) {
      case 'c':
        opt_config_path = optarg;
        break;
      case 'p':
        opt_p4info_path = optarg;
        break;
      case 'h':
        print_help(argv[0]);
        exit(0);
      case '?':
        if (optopt == 'c' || optopt == 'p') {
          fprintf(stderr, "Option -%c requires an argument.\n\n", optopt);
          print_help(argv[0]);
        } else if (isprint(optopt)) {
          fprintf(stderr, "Unknown option `-%c'.\n\n", optopt);
          print_help(argv[0]);
        } else {
          fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
          print_help(argv[0]);
        }
        return 1;
      default:
        abort();
    }
  }

  if (!opt_config_path) {
    fprintf(stderr, "Options -c is required.\n\n");
    print_help(argv[0]);
    return 1;
  }

  int extra_arg = 0;
  for (int index = optind; index < argc; index++) {
    fprintf(stderr, "Non-option argument: %s\n", argv[index]);
    extra_arg = 1;
  }
  if (extra_arg) {
    print_help(argv[0]);
    return 1;
  }

  return 0;
}

static
void *query_counter(general_switch_t gs) {
  printf("Start query thread!\n");
  char* counter_name = "MyIngress.myCounter";
  uint64_t packets;
  uint64_t bytes;
  GS_query_counter(gs, counter_name, 0, &packets, &bytes);
  printf("Counter %s: %lu packets(%lu bytes)\n", counter_name, packets, bytes);
  return NULL;
}

static void * add_table_entry(general_switch_t gs) {

  uint32_t ip = 0x0a000303;
  ip = ntohl(ip);  // "10.0.3.3"
  GS_match_field_lpm_t match_field_lpm = {
    .name = "hdr.ipv4.dstAddr",
    .value = (char*)&ip,
    .size = sizeof(ip),
    .plen = 32,
    .next = NULL
  };

  uint16_t port = 1;
  port = ntohs(port);
  GS_action_para_t action_para2 = {
    .name = "port",
    .value = (char*)&port,
    .size = sizeof(port),
    .next = NULL
  };

  unsigned char hw[6] = {0x00, 0xaa, 0xbb, 0x00, 0x00, 0x00};
  GS_action_para_t action_para1 = {
    .name = "dstAddr",
    .value = (char*)hw,
    .size = sizeof(hw),
    .next = &action_para2
  };

  GS_table_entry_t table_entry = {
    .table_name = "MyIngress.ipv4_lpm",
    .action_name = "MyIngress.ipv4_forward",
    .match_field_lpm = &match_field_lpm,
    .action_para = &action_para1
  };

  int rc = GS_add_table_entry(gs, &table_entry);
  printf("Add table entry end. Result: %d\n", rc);
  return NULL;
}

int main(int argc, char *argv[]) {
  if (parse_opts(argc, argv) != 0) return 1;
  const char* grpc_addr = "localhost:50051";
  general_switch_t gs = GS_connect(grpc_addr, opt_config_path, opt_p4info_path, 0);

  pthread_t thread_id;
  pthread_create(&thread_id, NULL, &query_counter, gs);
  // pthread_create(&thread_id, NULL, &add_table_entry, gs);
  add_table_entry(gs);

  GS_run(gs);
  printf("cannot get here\n");
}