/*
 * Copyright (C) 2025 Matej Bölcskei, ETH Zurich
 * Licensed under the GNU General Public License as published by the Free Software Foundation, version 3.
 * See LICENSE or <https://www.gnu.org/licenses/gpl-3.0.html> for details.
 * 
 * SPDX-License-Identifier: GPL-3.0-only
 */

#define _GNU_SOURCE

#include "microbenchmark.h"
#include "rubench_user.h"
#include "rubicon.h"

#include <stdio.h>

void pre_pcp_evict() {}

void microbenchmark_pcp_evict() { pcp_evict(); }

int post_pcp_evict() {
  int count = rubench_get_blocks();
  printf("Number of pages in the PCP: %d\n", count);
  return count != 0;
}

int main(void) {
  rubench_open();

  run_microbenchmark(1000, pre_pcp_evict, microbenchmark_pcp_evict,
                     post_pcp_evict);

  rubench_close();
  return 0;
}