This repository contains the Bolt toolchain which automatically generates performance contracts for Software NFs. 

# Prerequisites

The install script assumes you are using Ubuntu 16.04, though Debian may also work. You could also install the dependencies manually.

To run Bolt with hardware models (include DPDK in performance contract), you need a lot of RAM (100+ GB).

To run the NAT (but not to verify it), you need to set up hugepages for DPDK; see the [DPDK documentation](https://doc.dpdk.org/guides/linux_gsg/sys_reqs.html#linux-gsg-hugepages).


# Dependencies

Our dependencies, are modified versions of [KLEE](https://github.com/vignat/klee),
[KLEE-uClibc](https://github.com/vignat/klee-uclibc) and [Intel Pin](https://software.intel.com/sites/landingpage/pintool/docs/97554/Pin/html).


# Installation

Run `install.sh`, which will install the Bolt toolchain and create a file named `paths.sh` containing all necessary environment variables
(which is automatically added to your `~/.profile`).


# Compilation, Execution, Verification

```bash
# Compile VigNAT
$ cd nf/vignat
$ make

# Run it (this will print a help message)
$ ./build/nat

# Running Bolt on VigNAT
$ bash ../test-bolt.sh vignat
```


# Other information

Subdirectories have their own README files.

* nf - contains the libVig library of the data structures and all the NFs involved in the project
* install - patches and config files for the Bolt toolchain dependencies
* perf-contracts - contains the performance contracts for the data structure library. These are used as building blocks to generate performance contracts for entire NFs. 
