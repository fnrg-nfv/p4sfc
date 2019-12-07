#include "libswitch.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

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
void *query_counter(void* gs) {
  printf("Start query thread!\n");
  char* counter_name = "MyIngress.myCounter";
  uint64_t packets;
  uint64_t bytes;
  GS_query_counter(gs, counter_name, 0, &packets, &bytes);
  printf("Counter %s: %lu packets(%lu bytes)\n", counter_name, packets, bytes);
  return NULL;
}

int main(int argc, char *argv[]) {
  if (parse_opts(argc, argv) != 0) return 1;
  char* grpc_addr = "localhost:50051";
  void* gs = GS_connect(grpc_addr, opt_config_path, opt_p4info_path);

  pthread_t thread_id;
  pthread_create(&thread_id, NULL, &query_counter, gs);
  // pthread_join(thread_id, NULL);

  GS_run(gs);
  printf("cannot get here\n");
}