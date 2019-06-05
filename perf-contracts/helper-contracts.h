/* Performance contracts for Helper Functions */

#include "contract-params.h"

/* Perf contracts */

long ether_addr_hash_contract(std::string metric);

long ether_addr_eq_contract(std::string metric, long success);

long int_key_eq_contract(std::string metric, long success);

long ext_key_eq_contract(std::string metric, long success);

long int_key_hash_contract(std::string metric);

long ext_key_hash_contract(std::string metric);

long flow_cpy_contract(std::string metric, long recent);

long flow_destroy_contract(std::string metric);

long flow_extract_keys_contract(std::string metric);

long flow_pack_keys_contract(std::string metric);

/* Cstate contracts */

std::map<std::string, std::set<int>> ether_addr_hash_cstate_contract();

std::map<std::string, std::set<int>> ether_addr_eq_cstate_contract();

std::map<std::string, std::set<int>> int_key_hash_cstate_contract();

std::map<std::string, std::set<int>> ext_key_hash_cstate_contract();

std::map<std::string, std::set<int>> int_key_eq_cstate_contract();

std::map<std::string, std::set<int>> ext_key_eq_cstate_contract();

std::map<std::string, std::set<int>> flow_cpy_cstate_contract();

std::map<std::string, std::set<int>> flow_destroy_cstate_contract();

std::map<std::string, std::set<int>> flow_extract_keys_cstate_contract();

std::map<std::string, std::set<int>> flow_pack_keys_cstate_contract();