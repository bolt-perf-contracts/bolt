/* Hardware parameters used by all contracts */

/* Hardware parameters */

#define DRAM_LATENCY 200
#define L1_LATENCY 2

/* ABI */

#define _GLIBCXX_USE_CXX11_ABI 0

/* Common includes */
#include <map>
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <assert.h>
#include <vector>

/* Definition for cstate stitching function, required by all contracts */

std::map<std::string, std::set<int>>
add_cstate_dependency(std::map<std::string, std::set<int>> caller,
                      std::map<std::string, int> caller_state,
                      std::map<std::string, std::set<int>> callee);