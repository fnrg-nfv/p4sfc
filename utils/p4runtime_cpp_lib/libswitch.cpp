#include "general_switch.h"
#include "libswitch.h"

#include <PI/pi.h>

#include <boost/asio.hpp>

#include <ctype.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <streambuf>
#include <thread>


void* GS_connect(char* grpc_addr, char* config_path, char* p4info_path)
{
  boost::asio::io_service* io_service = new boost::asio::io_service();

  auto channel = grpc::CreateChannel(grpc_addr, grpc::InsecureChannelCredentials());
  int dev_id = 0;

  GeneralSwitch* general_switch = new GeneralSwitch(dev_id, io_service, channel);
  std::ifstream istream_config(config_path);
  std::string config((std::istreambuf_iterator<char>(istream_config)),
                      std::istreambuf_iterator<char>());
  int rc;
  if (!p4info_path) {
    rc = general_switch->assign(config, nullptr);
  } else {
    std::ifstream istream_p4info(p4info_path);
    std::string p4info_str((std::istreambuf_iterator<char>(istream_p4info)),
                           std::istreambuf_iterator<char>());
    rc = general_switch->assign(config, &p4info_str);
  }
  (void) rc;
  assert(rc == 0);
 
  return general_switch;
}

void GS_run(general_switch_t untyped_self)
{
  GeneralSwitch* typed_self = static_cast<GeneralSwitch*>(untyped_self);
  boost::asio::io_service::work work(*typed_self->io_service);
  std::cout << "io_service run" << std::endl;
  typed_self->io_service->run();
}

void GS_query_counter(general_switch_t untyped_self, char* counter_name, int index, uint64_t* packets, uint64_t* bytes)
{
  GeneralSwitch* typed_self = static_cast<GeneralSwitch*>(untyped_self);
  typed_self->query_counter(counter_name, index, packets, bytes);
}

// void GS_add_table_entry(general_switch_t untyped_self, )
// {
//   GeneralSwitch* typed_self = static_cast<GeneralSwitch*>(untyped_self);
//   typed_self->
// }
