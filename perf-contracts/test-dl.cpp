#include "klee/perf-contracts.h"
#include <assert.h>
#include <dlfcn.h>
#include <iostream>
#include <vector>

int main(int argc, char *argv[]) {
  assert(argc == 2);
  const char *plugin_file = argv[1];
  std::cout << "Loading plugin: " << plugin_file << std::endl;

  dlerror();
  const char *err = NULL;
  void *plugin = dlopen(plugin_file, RTLD_NOW);
  if ((err = dlerror())) {
    std::cout << "Error loading plugin: " << err << std::endl;
    exit(-1);
  }
  assert(plugin);

/* Testing init */ 
  decltype(&contract_init) init =
      (decltype(&contract_init))dlsym(plugin, STRINGIFY(contract_init));
  if ((err = dlerror())) {
    std::cout << "Error loading symbol: " << err << std::endl;
    exit(-1);
  }
  assert(init);

  init();

/* Testing get metrics */
  decltype(&contract_get_metrics) get_metrics =
      (decltype(&contract_get_metrics))dlsym(plugin, STRINGIFY(contract_get_metrics));
  if ((err = dlerror())) {
    std::cout << "Error loading symbol: " << err << std::endl;
    exit(-1);
  }
  assert(get_metrics);

  std::set<std::string> metrics=get_metrics();
/*  for(std::set<std::string>:: iterator i = metrics.begin();i!=metrics.end();i++)
  { 
     std::cout<<*i<<std::endl;
  }
*/
/* Testing has_contract */

 decltype(&contract_has_contract) has_contract =
      (decltype(&contract_has_contract))dlsym(plugin, STRINGIFY(contract_has_contract));
  if ((err = dlerror())) {
    std::cout << "Error loading symbol: " << err << std::endl;
    exit(-1);
  }
  assert(has_contract);
/*
  bool x = has_contract("dmap_put");
  bool y = has_contract("expire_items");
  bool z = has_contract("Hullo");

  if(x) std::cout<< "X" << std::endl;
  if(y) std::cout<<"Y" << std::endl;
  if(z) std::cout<<"Z" << std::endl; 
*/
/* Testing get_user_variables */

 std::map<std::string, std::string> user_variables;

 decltype(&contract_get_user_variables) get_user_variables =
      (decltype(&contract_get_user_variables))dlsym(plugin, STRINGIFY(contract_get_user_variables));
  if ((err = dlerror())) {
    std::cout << "Error loading symbol: " << err << std::endl;
    exit(-1);
  }
  assert(get_user_variables);
/*
  user_variables = get_user_variables();
  std::string s = "Num_hash_collisions_a";
  if(!user_variables.empty())
  { std::cout<< user_variables[s]<< std::endl;}
  else 
  {std::cout<<"No user variables"<< std::endl;}
*/
/* Testing get_optimization_variables */
  std::map<std::string, std::set<std::string>> optimization_variables;

 decltype(&contract_get_optimization_variables) get_optimization_variables =
      (decltype(&contract_get_optimization_variables))dlsym(plugin, STRINGIFY(contract_get_optimization_variables));
  if ((err = dlerror())) {
    std::cout << "Error loading symbol: " << err << std::endl;
    exit(-1);
  }
  assert(get_optimization_variables);
/*
  optimization_variables = get_optimization_variables();
  std::set<std::string> candidate_values = optimization_variables["expired_flows"];
  for(std::set<std::string>::iterator iter=candidate_values.begin(); iter!=candidate_values.end();++iter) 
  {  std::cout<<*iter<<std::endl;}
*/
/* Testing num_sub_contracts */


 decltype(&contract_num_sub_contracts) num_sub_contracts =
      (decltype(&contract_num_sub_contracts))dlsym(plugin, STRINGIFY(contract_num_sub_contracts));
  if ((err = dlerror())) {
    std::cout << "Error loading symbol: " << err << std::endl;
    exit(-1);
  }
  assert(num_sub_contracts);
  /*
  int num_subcontracts = num_sub_contracts("expire_items");
  std::cout << num_subcontracts << std::endl;
  */
/* Testing get_subcontract_constraints */
  std::string subcontract_constraints;

 decltype(&contract_get_subcontract_constraints) get_subcontract_constraints =
      (decltype(&contract_get_subcontract_constraints))dlsym(plugin, STRINGIFY(contract_get_subcontract_constraints));
  if ((err = dlerror())) {
    std::cout << "Error loading symbol: " << err << std::endl;
    exit(-1);
  }
  assert(get_subcontract_constraints);

  subcontract_constraints = get_subcontract_constraints("dmap_allocate",0);
  //std::cout << subcontract_constraints << std::endl;

/* Testing get_sub_contract_performance */
  
  decltype(&contract_get_sub_contract_performance) get_sub_contract_performance =
      (decltype(&contract_get_sub_contract_performance))dlsym(plugin, STRINGIFY(contract_get_sub_contract_performance));
  if ((err = dlerror())) {
    std::cout << "Error loading symbol: " << err << std::endl;
    exit(-1);
  }
  assert(get_sub_contract_performance);
  std::cout << "Calculating performance of libVig methods"<<std::endl;
  long perf;
  std::vector<std::string> fn_names;
  std::map<std::string, long> variables;
  std::map<std::string,int> subcontract_num;

  fn_names = {
              "dchain_allocate", "dchain_allocate_new_index", "dchain_rejuvenate_index",
              "map_allocate", "map_get", "map_put", "map_erase",              
              "dmap_allocate", "dmap_get_a", "dmap_get_b", "dmap_put", "dmap_erase","dmap_get_value",
              "expire_items", "expire_items_single_map",
              "vector_allocate","vector_borrow_full","vector_borrow_half","vector_return_full","vector_return_half",
             };

  variables = {
		
	{"map_capacity",{65536}},
	{"dmap_capacity",{65536}},
	{"map_occupancy",{65535}},
	{"dmap_occupancy",{65535}},
	{"map_allocation_succeeded",{1}},
        {"dmap_allocation_succeeded",{1}},
	{"map_has_this_key",{1}},
        {"dmap_has_this_key",{1}},
	{"Num_bucket_traversals",{1}},
        {"Num_bucket_traversals_a",{1}},
        {"Num_bucket_traversals_b",{1}},
        {"Num_hash_collisions",{0}},
        {"Num_hash_collisions_a",{0}},
        {"Num_hash_collisions_b",{0}},
	{"is_dchain_allocated",{1}},
	{"dchain_out_of_space",{0}},
	{"dchain_index_range",{65536}},
	{"expired_flows",{0}}
		};
  subcontract_num = {
	{"dchain_allocate",{0}},
	{"dchain_allocate_new_index",{0}},
	{"dchain_rejuvenate_index",{0}},
	{"map_allocate",{0}},
	{"map_get",{0}},
	{"map_put",{0}},
	{"map_erase",{0}},
	{"dmap_allocate",{0}},
	{"dmap_get_a",{0}},
	{"dmap_get_b",{0}},
	{"dmap_put",{0}},
	{"dmap_erase",{0}},
	{"dmap_get_value",{0}},
        {"expire_items",{0}},
	{"expire_items_single_map",{0}},
	{"vector_allocate",{0}},
	{"vector_borrow_full",{0}},
	{"vector_borrow_half",{0}},
	{"vector_return_full",{0}},
	{"vector_return_half",{0}}
             };

  for(std::vector<std::string>::iterator i = fn_names.begin();i!=fn_names.end();++i)
  {
	for(std::set<std::string>::iterator j = metrics.begin();j!=metrics.end();++j)
	{
		perf = get_sub_contract_performance(*i,subcontract_num[*i],*j,variables);
		std::cout<<*i<<" "<<*j<< " " <<perf<<std::endl;
	}
  }

  return 0;
}
