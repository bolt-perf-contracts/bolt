/* Performance contracts for Map-Impl functions */

#include "contract-params.h"
#include "helper-contracts.h"

/* Perf contracts */

long map_impl_init_contract(std::string metric, long success, long capacity);

long map_impl_put_contract(std::string metric, long recent, long num_traversals);

long map_impl_get_contract(std::string metric, long success, long recent,
                           long num_traversals, long num_collisions);

long map_impl_erase_contract(std::string metric, long recent, long num_traversals,
                             long num_collisions);

/* Cstate contracts */

std::map<std::string, std::set<int>>
map_impl_init_cstate_contract(long success, long capacity);

std::map<std::string, std::set<int>>
map_impl_put_cstate_contract(long num_traversals);

std::map<std::string, std::set<int>>
map_impl_get_cstate_contract(long success, long num_traversals,
                             long num_collisions);

std::map<std::string, std::set<int>>
map_impl_erase_cstate_contract(long num_traversals, long num_collisions);