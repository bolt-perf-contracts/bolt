#include "contract-params.h"
#include "map-impl-contracts.h"

/* Perf contracts */

long map_allocate_contract_0(std::string metric, std::vector<long> values);

long map_get_contract_0(std::string metric, std::vector<long> values);

long map_get_contract_1(std::string metric, std::vector<long> values);

long map_put_contract_0(std::string metric, std::vector<long> values);

long map_erase_contract_0(std::string metric, std::vector<long> values);

/* Cstate contracts */

std::map<std::string, std::set<int>>
map_allocate_cstate_contract_0(std::vector<long> values);

std::map<std::string, std::set<int>>
map_get_cstate_contract_0(std::vector<long> values);

std::map<std::string, std::set<int>>
map_get_cstate_contract_1(std::vector<long> values);

std::map<std::string, std::set<int>>
map_put_cstate_contract_0(std::vector<long> values);

std::map<std::string, std::set<int>>
map_erase_cstate_contract_0(std::vector<long> values);