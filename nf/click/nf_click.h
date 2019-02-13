#include "lib/nf_forward.h"
#include "lib/nf_util.h"

#ifdef KLEE_VERIFICATION
#include <klee/klee.h>
#else
#define klee_make_symbolic(x,y,z)
#endif

#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_tcp.h>

#include <stdlib.h>


// Assumes the mbuf is called 'mbuf'.
#define DROP_UNLESS(cond) if(!(cond)) { return mbuf->port; }

#define INPUT_PORT 0
#define OUTPUT_PORT 1


void nf_core_init(void)
{
	// Nothing
}

void nf_config_init(int argc, char** argv)
{
	// Nothing
}

void nf_config_set(void* value)
{
	// Nothing
}

void nf_config_cmdline_print_usage(void)
{
	// Nothing
}

void nf_print_config(void)
{
	// Nothing
}

#ifdef KLEE_VERIFICATION
void nf_loop_iteration_begin(unsigned lcore_id,
                             time_t time)
{
	// Nothing
}

void nf_add_loop_iteration_assumptions(unsigned lcore_id,
                                       time_t time)
{
	// Nothing
}

void nf_loop_iteration_end(unsigned lcore_id,
                           time_t time)
{
	// Nothing
}
#endif
