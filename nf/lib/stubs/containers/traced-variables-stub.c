#include "klee/klee.h"
#include <stdlib.h>
#include <string.h>

/* This is the same list of PCVs and OVs as in the contract file. This is unfortunate duplication and should be fixed */

int traced_variable_type(char *variable, char **type)
{
  *type = malloc(4 * sizeof(char));
  if (!strcmp(variable, "Num_bucket_traversals") ||
      !strcmp(variable, "Num_hash_collisions") ||
      !strcmp(variable, "map_occupancy") ||
      !strcmp(variable, "map_capacity") ||
      !strcmp(variable, "dmap_capacity") ||
      !strcmp(variable, "dmap_occupancy") ||
      !strcmp(variable, "expired_flows") ||
      !strcmp(variable, "backend_capacity") || 
      !strcmp(variable, "available_backends"))
  {
    strcpy(*type, "PCV");
    return 1;
  }
  else if (!strncmp(variable, "map_has_this_key_occurence", strlen("map_has_this_key_occurence")) || /* Has n versions */
           !strcmp(variable, "dmap_has_this_key") ||
           !strcmp(variable, "multi_stage_lookup") ||
           !strcmp(variable, "recent_flow") ||
           !strcmp(variable, "dchain_out_of_space"))
  {
    strcpy(*type, "SV");
    return 1;
  }
  else if (!strcmp(variable, "map") ||
           !strcmp(variable, "dmap") ||
           !strcmp(variable, "dchain") ||
           !strcmp(variable, "cht"))
  {
    strcpy(*type, "DS");
    return 1;
  }
  return 0;
}
