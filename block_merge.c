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

#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE 0x1000UL

static void *target;
static unsigned long target_phys;
static int file_fd;

void pre_block_merge() {
  target = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
  target_phys = rubench_va_to_pa(target);

  file_fd = open("/dev/shm", O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
}

void microbenchmark_block_merge() {
  const char *buf = "ffffffffffffffff";
  
  block_merge(target, 0);
  write(file_fd, buf, 8);
}

int post_block_merge() {
  void *file_ptr = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_POPULATE, file_fd, 0);
  unsigned long file_phys = rubench_va_to_pa(file_ptr);

  munmap(file_ptr, PAGE_SIZE);
  close(file_fd);

  printf("Target physical address: %lx\n", target_phys);
  printf("File physical address: %lx\n", file_phys);
  return target_phys != file_phys;
}

int main(void) {
  rubench_open();

  run_microbenchmark(100000, pre_block_merge, microbenchmark_block_merge,
                     post_block_merge);

  rubench_close();
  return 0;
}