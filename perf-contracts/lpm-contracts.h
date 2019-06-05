#include "contract-params.h"

/* Perf contracts */

long lpm_init_contract_0(std::string metric, std::vector<long> values);

long lpm_lookup_contract_0(std::string metric, std::vector<long> values);

/* Cstate contracts */

std::map<std::string, std::set<int>>
lpm_init_cstate_contract_0(std::vector<long> values);

std::map<std::string, std::set<int>>
lpm_lookup_cstate_contract_0(std::vector<long> values);