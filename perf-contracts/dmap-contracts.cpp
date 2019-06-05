#include "dmap-contracts.h"

/* Perf contracts */

long dmap_allocate_contract_0(std::string metric, std::vector<long> values)
{
  return 0;
}

long dmap_get_a_contract_0(std::string metric, std::vector<long> values)
{
  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  long recent = values[3];
  long constant, dependency;
  assert(success && !recent);
  if (metric == "instruction count")
  {
    constant = 25;
  }
  else if (metric == "memory instructions")
  {
    constant = 19;
  }
  else if (metric == "execution cycles")
  {
    constant = 0 * DRAM_LATENCY + 19 * L1_LATENCY + 8;
  }
  dependency = map_impl_get_contract(metric, success, recent, num_traversals,
                                     num_collisions);
  dependency += int_key_hash_contract(metric);
  return constant + dependency;
}

long dmap_get_a_contract_1(std::string metric, std::vector<long> values)
{
  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  long recent = values[3];
  long constant, dependency;
  assert(success && recent);
  if (metric == "instruction count")
  {
    constant = 25;
  }
  else if (metric == "memory instructions")
  {
    constant = 19;
  }
  else if (metric == "execution cycles")
  {
    constant = 0 * DRAM_LATENCY + 19 * L1_LATENCY + 8;
  }
  dependency = map_impl_get_contract(metric, success, recent, num_traversals,
                                     num_collisions);
  dependency += int_key_hash_contract(metric);
  return constant + dependency;
}

long dmap_get_a_contract_2(std::string metric, std::vector<long> values)
{
  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  long recent = values[3];
  long constant, dependency;
  assert(!success && !recent);
  if (metric == "instruction count")
  {
    constant = 25;
  }
  else if (metric == "memory instructions")
  {
    constant = 19;
  }
  else if (metric == "execution cycles")
  {
    constant = 0 * DRAM_LATENCY + 19 * L1_LATENCY + 8;
  }
  dependency = map_impl_get_contract(metric, success, recent, num_traversals,
                                     num_collisions);
  dependency += int_key_hash_contract(metric);
  return constant + dependency;
}

long dmap_get_b_contract_0(std::string metric, std::vector<long> values)
{
  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  long recent = values[3];
  long constant, dependency;
  assert(success && !recent);
  if (metric == "instruction count")
  {
    constant = 25;
  }
  else if (metric == "memory instructions")
  {
    constant = 19;
  }
  else if (metric == "execution cycles")
  {
    constant = 0 * DRAM_LATENCY + 19 * L1_LATENCY + 8;
  }
  dependency = map_impl_get_contract(metric, success, recent, num_traversals,
                                     num_collisions);
  dependency += ext_key_hash_contract(metric);
  return constant + dependency;
}

long dmap_get_b_contract_1(std::string metric, std::vector<long> values)
{
  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  long recent = values[3];
  long constant, dependency;
  assert(success && recent);
  if (metric == "instruction count")
  {
    constant = 25;
  }
  else if (metric == "memory instructions")
  {
    constant = 19;
  }
  else if (metric == "execution cycles")
  {
    constant = 0 * DRAM_LATENCY + 19 * L1_LATENCY + 8;
  }
  dependency = map_impl_get_contract(metric, success, recent, num_traversals,
                                     num_collisions);
  dependency += int_key_hash_contract(metric);
  return constant + dependency;
}
long dmap_get_b_contract_2(std::string metric, std::vector<long> values)
{
  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  long recent = values[3];
  long constant, dependency;
  assert(!success && !recent);
  if (metric == "instruction count")
  {
    constant = 25;
  }
  else if (metric == "memory instructions")
  {
    constant = 19;
  }
  else if (metric == "execution cycles")
  {
    constant = 0 * DRAM_LATENCY + 19 * L1_LATENCY + 8;
  }
  dependency = map_impl_get_contract(metric, success, recent, num_traversals,
                                     num_collisions);
  dependency += int_key_hash_contract(metric);
  return constant + dependency;
}
long dmap_get_value_contract_0(std::string metric, std::vector<long> values)
{
  long recent = values[0];
  assert(!recent);
  long constant, dependency;
  if (metric == "instruction count")
  {
    constant = 6;
  }
  else if (metric == "memory instructions")
  {
    constant = 3;
  }
  else if (metric == "execution cycles")
  {
    constant = 1 * DRAM_LATENCY + 2 * L1_LATENCY + 3;
  }
  dependency = flow_cpy_contract(metric, recent);
  return constant + dependency;
}
long dmap_get_value_contract_1(std::string metric, std::vector<long> values)
{
  long recent = values[0];
  assert(recent);
  long constant, dependency;
  if (metric == "instruction count")
  {
    constant = 6;
  }
  else if (metric == "memory instructions")
  {
    constant = 3;
  }
  else if (metric == "execution cycles")
  {
    constant = 1 * DRAM_LATENCY + 2 * L1_LATENCY + 3;
  }
  dependency = flow_cpy_contract(metric, recent);
  return constant + dependency;
}

long dmap_put_contract_0(std::string metric, std::vector<long> values)
{
  long num_traversals = values[0];
  long recent = values[1];
  assert(!recent);
  long constant, dependency;
  if (metric == "instruction count")
  {
    constant = 65;
  }
  else if (metric == "memory instructions")
  {
    constant = 45;
  }
  else if (metric == "execution cycles")
  {
    constant = 0 * DRAM_LATENCY + 45 * L1_LATENCY + 27;
  }
  dependency = map_impl_put_contract(metric, recent, num_traversals);
  dependency += map_impl_put_contract(metric, recent, num_traversals);
  dependency += flow_cpy_contract(metric, recent);
  dependency += flow_extract_keys_contract(metric);
  dependency += flow_pack_keys_contract(metric);
  dependency += int_key_hash_contract(metric);
  dependency += ext_key_hash_contract(metric);
  return constant + dependency;
}
long dmap_put_contract_1(std::string metric, std::vector<long> values)
{
  long num_traversals = values[0];
  long recent = values[1];
  assert(recent);
  long constant, dependency;
  if (metric == "instruction count")
  {
    constant = 65;
  }
  else if (metric == "memory instructions")
  {
    constant = 45;
  }
  else if (metric == "execution cycles")
  {
    constant = 0 * DRAM_LATENCY + 45 * L1_LATENCY + 27;
  }
  dependency = map_impl_put_contract(metric, recent, num_traversals);
  dependency += map_impl_put_contract(metric, recent, num_traversals);
  dependency += flow_cpy_contract(metric, recent);
  dependency = flow_extract_keys_contract(metric);
  dependency += flow_pack_keys_contract(metric);
  dependency += int_key_hash_contract(metric);
  dependency += ext_key_hash_contract(metric);
  return constant + dependency;
}

long dmap_erase_contract_0(std::string metric, std::vector<long> values)
{
  long num_traversals = values[0];
  long num_collisions = values[1];
  long recent = values[2];
  assert(!recent);
  long constant, dependency;
  if (metric == "instruction count")
  {
    constant = 72;
  }
  else if (metric == "memory instructions")
  {
    constant = 48;
  }
  else if (metric == "execution cycles")
  {
    constant = 0 * DRAM_LATENCY + 48 * L1_LATENCY + 31;
  }
  dependency =
      map_impl_erase_contract(metric, recent, num_traversals, num_collisions);
  dependency +=
      map_impl_erase_contract(metric, recent, num_traversals, num_collisions);
  dependency += flow_extract_keys_contract(metric);
  dependency += flow_pack_keys_contract(metric);
  dependency += flow_pack_keys_contract(metric);
  dependency += int_key_hash_contract(metric);
  dependency += ext_key_hash_contract(metric);
  dependency += flow_destroy_contract(metric);
  return constant + dependency;
}
long dmap_erase_contract_1(std::string metric, std::vector<long> values)
{
  long num_traversals = values[0];
  long num_collisions = values[1];
  long recent = values[2];
  assert(recent);
  long constant, dependency;
  if (metric == "instruction count")
  {
    constant = 72;
  }
  else if (metric == "memory instructions")
  {
    constant = 48;
  }
  else if (metric == "execution cycles")
  {
    constant = 0 * DRAM_LATENCY + 48 * L1_LATENCY + 31;
  }
  dependency =
      map_impl_erase_contract(metric, recent, num_traversals, num_collisions);
  dependency +=
      map_impl_erase_contract(metric, recent, num_traversals, num_collisions);
  dependency += flow_extract_keys_contract(metric);
  dependency += flow_pack_keys_contract(metric);
  dependency += flow_pack_keys_contract(metric);
  dependency += int_key_hash_contract(metric);
  dependency += ext_key_hash_contract(metric);
  dependency += flow_destroy_contract(metric);
  return constant + dependency;
}

/* Cstate contracts */

std::map<std::string, std::set<int>>
dmap_allocate_cstate_contract_0(std::vector<long> values)
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}

std::map<std::string, std::set<int>>
dmap_get_a_cstate_contract_0(std::vector<long> values)
{

  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  assert(success);

  std::map<std::string, std::set<int>> cstate;
  cstate["rsp"] = {-8, -16, -24, -32, -40, -48, -56, -64};
  cstate["rbx"] = {80, 168, 56, 48, 40, 32, 64, 72};
  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -64;
  dependency_calls["rbx"] = 0;
  cstate =
      add_cstate_dependency(cstate, dependency_calls,
                            map_impl_get_cstate_contract(
                                success, num_traversals, num_collisions));
  dependency_calls["rsp"] = -32;
  cstate = add_cstate_dependency(cstate, dependency_calls,
                                 int_key_hash_cstate_contract()); /*For vignat*/
  return cstate;
}

std::map<std::string, std::set<int>>
dmap_get_a_cstate_contract_1(std::vector<long> values)
{

  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  assert(success == 0);

  std::map<std::string, std::set<int>> cstate;
  cstate["rsp"] = {-8, -16, -24, -32, -40, -48, -56, -64};
  cstate["rbx"] = {80, 168, 56, 48, 40, 32, 64, 72};
  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -64;
  dependency_calls["rbx"] = 0;
  cstate =
      add_cstate_dependency(cstate, dependency_calls,
                            map_impl_get_cstate_contract(
                                success, num_traversals, num_collisions));
  dependency_calls["rsp"] = -32;
  cstate = add_cstate_dependency(cstate, dependency_calls,
                                 int_key_hash_cstate_contract()); /*For vignat*/
  return cstate;
}

std::map<std::string, std::set<int>>
dmap_get_b_cstate_contract_0(std::vector<long> values)
{

  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  assert(success);

  std::map<std::string, std::set<int>> cstate;
  cstate["rsp"] = {-8, -16, -24, -32, -40, -48, -56, -64};
  cstate["rbx"] = {136, 168, 112, 104, 96, 88, 120, 128};
  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -64;
  dependency_calls["rbx"] = 0;
  cstate =
      add_cstate_dependency(cstate, dependency_calls,
                            map_impl_get_cstate_contract(
                                success, num_traversals, num_collisions));
  dependency_calls["rsp"] = -32;
  cstate = add_cstate_dependency(cstate, dependency_calls,
                                 ext_key_hash_cstate_contract()); /*For vignat*/
  return cstate;
}

std::map<std::string, std::set<int>>
dmap_get_b_cstate_contract_1(std::vector<long> values)
{

  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  assert(success == 0);

  std::map<std::string, std::set<int>> cstate;
  cstate["rsp"] = {-8, -16, -24, -32, -40, -48, -56, -64};
  cstate["rbx"] = {136, 168, 112, 104, 96, 88, 120, 128};
  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -64;
  dependency_calls["rbx"] = 0;
  cstate =
      add_cstate_dependency(cstate, dependency_calls,
                            map_impl_get_cstate_contract(
                                success, num_traversals, num_collisions));
  dependency_calls["rsp"] = -32;
  cstate = add_cstate_dependency(cstate, dependency_calls,
                                 ext_key_hash_cstate_contract()); /*For vignat*/
  return cstate;
}
std::map<std::string, std::set<int>>
dmap_get_value_cstate_contract_0(std::vector<long> values)
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}

std::map<std::string, std::set<int>>
dmap_put_cstate_contract_0(std::vector<long> values)
{

  long num_traversals = values[0];

  std::map<std::string, std::set<int>> cstate;
  cstate["rsp"] = {-8, -16, -24, -32, -40, -48, -56, -64, -72, -80, -88, -96};
  cstate["rbx"] = {8, 168, 144, 80, 56, 48, 40, 32, 64,
                   136, 112, 104, 96, 88, 120, 160, 152};
  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -64;
  dependency_calls["rbx"] = 0;
  cstate = add_cstate_dependency(cstate, dependency_calls,
                                 flow_cpy_cstate_contract()); /*For vignat*/
  cstate =
      add_cstate_dependency(cstate, dependency_calls,
                            flow_extract_keys_cstate_contract()); /*For vignat*/
  cstate = add_cstate_dependency(cstate, dependency_calls,
                                 int_key_hash_cstate_contract()); /*For vignat*/
  cstate = add_cstate_dependency(cstate, dependency_calls,
                                 ext_key_hash_cstate_contract()); /*For vignat*/
  cstate =
      add_cstate_dependency(cstate, dependency_calls,
                            flow_pack_keys_cstate_contract()); /*For vignat*/

  dependency_calls["rsp"] = -96;
  cstate = add_cstate_dependency(
      cstate, dependency_calls,
      map_impl_put_cstate_contract(num_traversals)); /*For vignat*/
  cstate = add_cstate_dependency(
      cstate, dependency_calls,
      map_impl_put_cstate_contract(num_traversals)); /*For vignat*/
  return cstate;
}

std::map<std::string, std::set<int>>
dmap_erase_cstate_contract_0(std::vector<long> values)
{
  std::map<std::string, std::set<int>> cstate;
  return cstate;
}