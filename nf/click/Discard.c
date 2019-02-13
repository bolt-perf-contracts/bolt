#include "click/nf_click.h"


int nf_core_process(struct rte_mbuf* mbuf, time_t now)
{
	// inputPortName(0)
	DROP_UNLESS(mbuf->port == INPUT_PORT);

	// Fail("Unexpected packet dropped @ " + getName)
	return mbuf->port;
}
