#include "map-contracts.h"

/* Perf contracts */

long map_allocate_contract_0(std::string metric, std::vector<long> values)
{
  return 0;
}
long map_get_contract_0(std::string metric, std::vector<long> values)
{
  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  long constant, dependency;
  assert(success);
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
  dependency = map_impl_get_contract(
      metric, success, 1, num_traversals,
      num_collisions); /* This currently uses the NAT equality fn */
  dependency +=
      ether_addr_hash_contract(metric); /* Bridge, need separate for LB */
  return constant + dependency;
}
long map_get_contract_1(std::string metric, std::vector<long> values)
{
  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  long constant, dependency;
  assert(success == 0);
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
  dependency = map_impl_get_contract(
      metric, success, 1, num_traversals,
      num_collisions); /* This currently uses the NAT equality fn */
  dependency +=
      ether_addr_hash_contract(metric); /* Bridge need separate for LB */
  return constant + dependency;
}

long map_put_contract_0(std::string metric, std::vector<long> values)
{
  long num_traversals = values[0];
  long constant, dependency;
  if (metric == "instruction count")
  {
    constant = 26;
  }
  else if (metric == "memory instructions")
  {
    constant = 19;
  }
  else if (metric == "execution cycles")
  {
    constant = 0 * DRAM_LATENCY + 19 * L1_LATENCY + 9;
  }
  dependency = map_impl_put_contract(metric, 1, num_traversals);
  dependency +=
      ether_addr_hash_contract(metric); /* Bridge need separate for LB */
  return constant + dependency;
}

long map_erase_contract_0(std::string metric, std::vector<long> values)
{
  long num_traversals = values[0];
  long num_collisions = values[1];
  long constant, dependency;
  if (metric == "instruction count")
  {
    constant = 26;
  }
  else if (metric == "memory instructions")
  {
    constant = 19;
  }
  else if (metric == "execution cycles")
  {
    constant = 0 * DRAM_LATENCY + 19 * L1_LATENCY + 9;
  }
  dependency = map_impl_erase_contract(metric, 1, num_traversals, num_collisions);
  dependency +=
      ether_addr_hash_contract(metric); /* Bridge need separate for LB */
  return constant + dependency;
}

/* Cstate contracts */

std::map<std::string, std::set<int>>
map_allocate_cstate_contract_0(std::vector<long> values)
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}
std::map<std::string, std::set<int>>
map_get_cstate_contract_0(std::vector<long> values)
{

  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  assert(success);
  std::map<std::string, std::set<int>> cstate;

  cstate["rsp"] = {-8, -16, -24, -32, -40, -48, -56, -64};
  cstate["rbx"] = {40, 24, 16, 8, 32, 0};

  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -64;
  dependency_calls["rbx"] = 0;
  cstate = add_cstate_dependency(
      cstate, dependency_calls,
      map_impl_get_cstate_contract(success, num_traversals, num_collisions));
  dependency_calls["rsp"] = -32;
  cstate =
      add_cstate_dependency(cstate, dependency_calls,
                            ether_addr_hash_cstate_contract()); /*For bridge*/

  return cstate;
}
std::map<std::string, std::set<int>>
map_get_cstate_contract_1(std::vector<long> values)
{

  long num_traversals = values[0];
  long num_collisions = values[1];
  long success = values[2];
  assert(success == 0);
  std::map<std::string, std::set<int>> cstate;

  cstate["rsp"] = {-8, -16, -24, -32, -40, -48, -56, -64};
  cstate["rbx"] = {40, 24, 16, 8, 32, 0};
  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -64;
  dependency_calls["rbx"] = 0;
  cstate = add_cstate_dependency(
      cstate, dependency_calls,
      map_impl_get_cstate_contract(success, num_traversals, num_collisions));
  dependency_calls["rsp"] = -32;
  cstate =
      add_cstate_dependency(cstate, dependency_calls,
                            ether_addr_hash_cstate_contract()); /*For bridge*/

  return cstate;
}

std::map<std::string, std::set<int>>
map_put_cstate_contract_0(std::vector<long> values)
{

  long num_traversals = values[0];
  std::map<std::string, std::set<int>> cstate;

  cstate["rsp"] = {-8, -16, -24, -32, -40, -48, -56, -64};
  cstate["rbx"] = {40, 24, 16, 8, 32, 0, 44};
  std::map<std::string, int> dependency_calls;
  dependency_calls["rsp"] = -64;
  dependency_calls["rbx"] = 0;
  cstate = add_cstate_dependency(cstate, dependency_calls,
                                 map_impl_put_cstate_contract(num_traversals));
  dependency_calls["rsp"] = -32;
  cstate =
      add_cstate_dependency(cstate, dependency_calls,
                            ether_addr_hash_cstate_contract()); /*For bridge*/

  return cstate;
}

std::map<std::string, std::set<int>>
map_erase_cstate_contract_0(std::vector<long> values)
{
  std::map<std::string, std::set<int>> cstate;
  return cstate;
}
