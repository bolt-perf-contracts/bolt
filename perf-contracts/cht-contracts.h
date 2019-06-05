#include "contract-params.h"
#include "vector-contracts.h"
#include "dchain-contracts.h"

/* Perf contracts */
long lb_find_preferred_available_backend_contract_0(std::string metric,
                                                    std::vector<long> values);

/* Cstate contracts */
std::map<std::string, std::set<int>>
lb_find_preferred_available_backend_cstate_contract_0(
    std::vector<long> values);