# Rubicon Microbenchmarks
This repository provides a collection of microbenchmarks designed to evaluate the three core mechanisms that form the foundation of Rubicon. The microbenchmarks directly correspond to the ones detailed in section 8.1 of our paper ["Rubicon: Precise Microarchitectural Attacks with Page-Granular Massaging"](https://comsec.ethz.ch/wp-content/files/rubicon_eurosp25.pdf). 

## Requirements
The microbenchmarks require the following system components and development tools:

- **Linux Kernel**: Tested with versions 5.4, 5.8, 5.15, and 6.8. Other versions should work as long as they are not unreasonably old.
- **Linux Headers**: Required for compiling the `rubench` kernel module.
- **GCC**: Used to compile the kernel module. We used version 12.3.0, but other versions should work as well.
- **Clang**: Required for building user-space components. We used version 10.0.0. Again, other versions should work as well.

## Rubench Kernel Module

After ensuring that the above components are installed, you can build and load the `rubench` kernel module by running the following command:
```bash
(cd rubench/ && make && make load)
```
The `rubench` kernel module provides access to the internal state of the allocator, which is required to verify the success of the massaging mechanisms. For example, it allows querying the number of pages on a specific PCP list, which is necessary to conclusively verify whether the PCP Evict has successfully evicted all pages. User-space components can communicate with the kernel module through the `/dev/rubench` device file using `ioctl` calls. A set of functions to simplify the interface for these `ioctl` calls is provided in `src/rubench_user.c`.

## Microbenchmarks
The microbenchmarks use a generic testing function, run_microbenchmark(), which takes three function pointers: a preparation function (sets up the test, e.g., retrieves the target page's physical address), a mechanism function (executes the massaging mechanism), and a verification function (checks success, e.g., compares the victim physical address to target). run_microbenchmark() runs these in a loop for a configurable number of iterations, timing each run (mechanism function only), verifying success, and collecting statistics on performance and success rate.

Each microbenchmark is implemented in a separate file in the repository root: `pcp_evict.c`, `block_merge.c`, and `migratetype_escalation.c`. These correspond one-to-one to the mechanisms (and their respective microbenchmarks) described in the paper. These implementations follow a common structure: define the three functions and invoke run_microbenchmark().

To compile the microbenchmarks, simply run:
```bash
make
```
This creates three microbenchmark executables: `pcp_evict`, `block_merge`, and `migratetype_escalation`.

To run the microbenchmarks, you can simply execute the corresponding binary. For example, to run the `block_merge` microbenchmark, use:
```bash
./block_merge
```
This executes the microbenchmark and prints the results to the console:
```
Round 0
Start time: 77126.325414727
End time: 77126.325421842
Target physical address: 166efb000
File physical address: 166efb000
PASS: 7115 ns
Round 1
Start time: 77126.325442044
End time: 77126.325445199
Target physical address: 166efb000
File physical address: 166efb000
PASS: 3155 ns
.
.
.
Number of failed tests: 0
Average time taken: 4297 ns
```

## Citing our Work
To cite Rubicon in academic papers, please use the following BibTeX entry:

```
@inproceedings{boelcskei_rubicon_2025,
	title = {{Rubicon: Precise Microarchitectural Attacks with Page-Granular Massaging}}, 
	url = {Paper=https://comsec.ethz.ch/wp-content/files/rubicon_eurosp25.pdf},
	booktitle = {{EuroS\&P}},
	author = {Bölcskei, Matej and Jattke, Patrick and Wikner, Johannes and Razavi, Kaveh},
	month = jun,
	year = {2025},
	keywords = {dir\_os, type\_conf}
}
```