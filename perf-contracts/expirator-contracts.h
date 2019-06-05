#include "contract-params.h"
#include "vector-contracts.h"
#include "dchain-contracts.h"
#include "map-contracts.h"
#include "dmap-contracts.h"

/* Perf contracts */

long expire_items_contract_0(std::string metric, std::vector<long> values);

long expire_items_single_map_contract_0(std::string metric,
                                        std::vector<long> values);

/* Cstate contracts */

std::map<std::string, std::set<int>>
expire_items_cstate_contract_0(std::vector<long> values);

std::map<std::string, std::set<int>>
expire_items_single_map_cstate_contract_0(std::vector<long> values);