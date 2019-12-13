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


void* GS_connect(char* grpc_addr, char* config_path, char* p4info_path, int dev_id)
{
  boost::asio::io_service* io_service = new boost::asio::io_service();

  auto channel = grpc::CreateChannel(grpc_addr, grpc::InsecureChannelCredentials());

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

void GS_query_counter(general_switch_t untyped_self,
                      char* counter_name,
                      int index,
                      uint64_t* packets,
                      uint64_t* bytes)
{
  GeneralSwitch* typed_self = static_cast<GeneralSwitch*>(untyped_self);
  typed_self->query_counter(counter_name, index, packets, bytes);
}

int GS_add_table_entry(general_switch_t untyped_self, GS_table_entry_t *table_entry)
{
  GeneralSwitch* typed_self = static_cast<GeneralSwitch*>(untyped_self);
  pi_p4info_t *p4info = typed_self->p4info;
  pi_p4_id_t t_id = pi_p4info_table_id_from_name(p4info, table_entry->table_name);
  pi_p4_id_t a_id = pi_p4info_action_id_from_name(p4info, table_entry->action_name);

  // printf("t_id: %d, a_id: %d\n", t_id, a_id);

  p4v1::TableEntry match_action_entry;
  match_action_entry.set_table_id(t_id);

  for (GS_match_field_lpm_t *match_field_lpm = table_entry->match_field_lpm;
       match_field_lpm; match_field_lpm = match_field_lpm->next)
  {
    auto mf = match_action_entry.add_match();
    mf->set_field_id(pi_p4info_table_match_field_id_from_name(p4info, t_id, match_field_lpm->name));
    auto mf_lpm = mf->mutable_lpm();
    auto value = std::string(match_field_lpm->value, match_field_lpm->size);
    mf_lpm->set_value(value);
    mf_lpm->set_prefix_len(match_field_lpm->plen);
    // std::cout << "match_field_lpm: " << match_field_lpm->name << "\t"
    //           << value << "\t"
    //           << match_field_lpm->plen << std::endl;
  }

  for (GS_match_field_exact_t *match_field_exact = table_entry->match_field_exact;
       match_field_exact; match_field_exact = match_field_exact->next)
  {
    auto mf = match_action_entry.add_match();
    mf->set_field_id(pi_p4info_table_match_field_id_from_name(p4info, t_id, match_field_exact->name));
    auto mf_exact = mf->mutable_exact();
    auto value = std::string(match_field_exact->value, match_field_exact->size);
    mf_exact->set_value(value);
  }

  auto entry = match_action_entry.mutable_action();
  auto action = entry->mutable_action();
  action->set_action_id(a_id);
  for (GS_action_para_t *action_para = table_entry->action_para;
       action_para; action_para = action_para->next)
  {
    auto param = action->add_params();
    auto value = std::string(action_para->value, action_para->size);
    param->set_param_id(pi_p4info_action_param_id_from_name(p4info, a_id, action_para->name));
    param->set_value(value);
    // std::cout << "para: " << action_para->name << "\t"
    //           << value << std::endl;
  }

  return typed_self->add_one_entry(&match_action_entry);
}
