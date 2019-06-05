#include "map-impl-contracts.h"

/* Perf contracts */

long map_impl_init_contract(std::string metric, long success, long capacity)
{
  if (success)
  {
    return (2 * capacity + 1) *
           DRAM_LATENCY; // Have not gone through patterns here. In progress
  }
  else
    return 0;
}

long map_impl_put_contract(std::string metric, long recent, long num_traversals)
{
  long constant, dynamic;
  if (metric == "instruction count")
  {
    dynamic = 0;
    if (num_traversals == 1)
    {
      constant = 39;
    }
    else
    {
      constant = 38;
      dynamic = 13 * (num_traversals - 1);
    }
  }
  else if (metric == "memory instructions")
  {
    dynamic = 0;
    if (num_traversals == 1)
    {
      constant = 20;
    }
    else
    {
      constant = 20;
      dynamic = 2 * (num_traversals - 1);
    }
  }
  else if (metric == "execution cycles")
  {
    constant = 16 * L1_LATENCY + 32;
    if (recent)
      constant += 4 * L1_LATENCY;
    else
      constant += 4 * DRAM_LATENCY;

    dynamic = 0;
    if (num_traversals > 1)
    {
      num_traversals--;
      dynamic = ((2 * num_traversals) / 16 + 2) * DRAM_LATENCY +
                (((30 * num_traversals) / 16) - 2) * L1_LATENCY +
                18 * num_traversals; // The additional 2 is for misalignments
    }
  }
  return constant + dynamic;
}

long map_impl_get_contract(std::string metric, long success, long recent,
                           long num_traversals, long num_collisions)
{
  long constant, dynamic;
  constant = 0;
  dynamic = 0;
  long rem_traversals;
  if (metric == "instruction count")
  {
    if (success)
    {
      if (num_traversals == 1)
      {
        constant = 65 + int_key_eq_contract(metric, success);
      }
      else
      {
        constant = 65 + int_key_eq_contract(metric, success);
        dynamic += num_collisions * (28 + int_key_eq_contract(metric, 0));
        rem_traversals = num_traversals - num_collisions - 1;
        dynamic += rem_traversals * 18;
      }
    }
    else
    {
      if (num_collisions == 0 &&
          num_traversals != 65536) // This should be capacity
      {
        constant = 47;
      }
      else if (num_traversals != 65536)
      {
        constant = 49;
      }
      else
      {
        constant = 52;
      }
      dynamic += num_collisions * (28 + int_key_eq_contract(metric, 0));
      rem_traversals = num_traversals - num_collisions - 1;
      dynamic += rem_traversals * 18;
    }
  }
  else if (metric == "memory instructions")
  {
    if (success)
    {
      if (num_traversals == 1)
      {
        constant = 37 + int_key_eq_contract(metric, success);
      }
      else
      {
        constant = 37 + int_key_eq_contract(metric, success);
        dynamic += num_collisions * (14 + int_key_eq_contract(metric, 0));
        rem_traversals = num_traversals - num_collisions - 1;
        dynamic += rem_traversals * 4;
      }
    }
    else
    {
      if (num_collisions == 0 &&
          num_traversals != 65536) // This should be capacity
      {
        constant = 22;
      }
      else if (num_traversals != 65536)
      {
        constant = 23;
      }
      else
      {
        constant = 23;
      }
      dynamic += num_collisions * (14 + int_key_eq_contract(metric, 0));
      rem_traversals = num_traversals - num_collisions - 1;
      dynamic += rem_traversals * 4;
    }
  }
  else if (metric == "execution cycles")
  {
    if (success)
    {
      if (recent)
        constant = 4 * L1_LATENCY;
      else
        constant = 4 * DRAM_LATENCY;

      constant += 33 * L1_LATENCY + 31 +
                  int_key_eq_contract(metric, success);
    }
    else
    {
      constant = 2 * DRAM_LATENCY + 20 * L1_LATENCY +
                 32; // Can still optimise across static instances
    }
    dynamic = 0;
    if (num_traversals > 1)
    {
      num_traversals--;
      dynamic = ((3 * num_traversals) / 16 + 2) * DRAM_LATENCY +
                (((45 * num_traversals) / 16) - 2) * L1_LATENCY +
                18 * num_traversals; // The additional 2 is for misalignments
    }
    if (num_collisions > 0)
    {
      dynamic += 1 * DRAM_LATENCY + ((1 * num_collisions) / 16) * DRAM_LATENCY +
                 (24 * num_collisions) * L1_LATENCY +
                 15 * num_collisions; // Keys are 16B, so always cache aligned
    }
  }
  return constant + dynamic;
}

long map_impl_erase_contract(std::string metric, long recent, long num_traversals,
                             long num_collisions)
{
  long constant, dynamic;
  constant = 0;
  dynamic = 0;
  long rem_traversals;
  if (metric == "instruction count")
  {
    if (num_traversals == 1)
    {
      constant = 69 + int_key_eq_contract(metric, 1);
    }
    else
    {
      constant = 69 + int_key_eq_contract(metric, 1);
      rem_traversals = num_traversals - num_collisions - 1;
      dynamic += num_collisions * (39 + int_key_eq_contract(metric, 0));
      dynamic += 19 * rem_traversals;
    }
  }
  else if (metric == "memory instructions")
  {
    if (num_traversals == 1)
    {
      constant = 43 + int_key_eq_contract(metric, 1);
    }
    else
    {
      constant = 43 + int_key_eq_contract(metric, 1);
      rem_traversals = num_traversals - num_collisions - 1;
      dynamic += num_collisions * (21 + int_key_eq_contract(metric, 0));
      dynamic += 4 * rem_traversals;
    }
  }
  else if (metric == "execution cycles")
  {
    constant = 39 * L1_LATENCY + 31 +
               int_key_eq_contract(metric, 1);
    if (recent)
      constant += 4 * L1_LATENCY;
    else
      constant += 4 * DRAM_LATENCY;
    if (num_traversals > 1)
    {
      num_traversals--;
      dynamic = ((3 * num_traversals) / 16 + 2) * DRAM_LATENCY +
                (((61 * num_traversals) / 16) - 2) * L1_LATENCY +
                17 * num_traversals;
    }
    if (num_collisions > 0)
    {
      dynamic += 1 * DRAM_LATENCY + ((1 * num_collisions) / 16) * DRAM_LATENCY +
                 (24 * num_collisions) * L1_LATENCY +
                 15 * num_collisions; // Keys are 16B, so always cache aligned
    }
  }
  return constant + dynamic;
}

/* Cstate contracts */

std::map<std::string, std::set<int>>
map_impl_init_cstate_contract(long success, long capacity)
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}

std::map<std::string, std::set<int>>
map_impl_put_cstate_contract(long num_traversals)
{

  std::map<std::string, std::set<int>> cstate;
  cstate["rsp"] = {-8, -16, -24, -32, -40, 32, 16, 24};
  return cstate;
}

std::map<std::string, std::set<int>>
map_impl_get_cstate_contract(long success, long num_traversals,
                             long num_collisions)
{

  std::map<std::string, std::set<int>> cstate;
  if (success)
  {
    cstate["rsp"] = {-8, -16, -24, -32, -40, 32, 16, -80, -64, -56,
                     -80, -84, -88, -96, -72, -104, -64, -48, 24};
    std::map<std::string, int> dependency_calls;
    dependency_calls["rsp"] = -104;
    cstate = add_cstate_dependency(
        cstate, dependency_calls,
        int_key_eq_cstate_contract()); /*This is for the NAT*/
  }
  else
  {
    cstate["rsp"] = {-8, -16, -24, -32, -40, 32, 16, -80, -64, -56};
  }
  return cstate;
}

std::map<std::string, std::set<int>>
map_impl_erase_cstate_contract(long num_traversals, long num_collisions)
{

  std::map<std::string, std::set<int>> cstate;
  return cstate;
}