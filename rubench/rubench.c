/*
 * Copyright (C) 2025 Matej Bölcskei, ETH Zurich
 * Licensed under the GNU General Public License as published by the Free Software Foundation, version 3.
 * See LICENSE or <https://www.gnu.org/licenses/gpl-3.0.html> for details.
 * 
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "rubench.h"

#include <asm/io.h>
#include <linux/cpu.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0) && \
    LINUX_VERSION_CODE < KERNEL_VERSION(5, 16, 0)
static inline unsigned int order_to_pindex(int migratetype, int order) {
  int base = order;

#ifdef CONFIG_TRANSPARENT_HUGEPAGE
  if (order > PAGE_ALLOC_COSTLY_ORDER) {
    VM_BUG_ON(order != pageblock_order);
    base = PAGE_ALLOC_COSTLY_ORDER + 1;
  }
#else
  VM_BUG_ON(order > PAGE_ALLOC_COSTLY_ORDER);
#endif

  return (MIGRATE_PCPTYPES * base) + migratetype;
}
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6, 8, 0) && \
    LINUX_VERSION_CODE < KERNEL_VERSION(6, 9, 0)
static inline unsigned int order_to_pindex(int migratetype, int order) {
#ifdef CONFIG_TRANSPARENT_HUGEPAGE
  if (order > PAGE_ALLOC_COSTLY_ORDER) {
    VM_BUG_ON(order != pageblock_order);
    return NR_LOWORDER_PCP_LISTS;
  }
#else
  VM_BUG_ON(order > PAGE_ALLOC_COSTLY_ORDER);
#endif

  return (MIGRATE_PCPTYPES * order) + migratetype;
}
#else
#error "Unsupported kernel version! Only 5.15.0 and 6.8.0 are supported."
#endif

static int rubench_open(struct inode *inode, struct file *file);
static int rubench_release(struct inode *inode, struct file *file);
static long rubench_ioctl(struct file *file, unsigned int cmd,
                          unsigned long arg);

static const struct file_operations rubench_fops = {
    .owner = THIS_MODULE,
    .open = rubench_open,
    .release = rubench_release,
    .unlocked_ioctl = rubench_ioctl,
};

static int rubench_major;
static struct class *rubench_class;
static struct device *rubench_device;

static int rubench_open(struct inode *inode, struct file *file) {
  pr_info("Rubench module opened\n");
  return 0;
}

static int rubench_release(struct inode *inode, struct file *file) {
  pr_info("Rubench module released\n");
  return 0;
}

static long rubench_ioctl(struct file *file, unsigned int cmd,
                          unsigned long arg) {
  switch (cmd) {
    case RUBENCH_GET_BLOCKS: {
      struct rubench_get_blocks_data data;
      int cpu;
      struct zone *zone;
      struct per_cpu_pages *pcp;
      int pindex;
      struct list_head *head;
      int count;
      struct page *page, *next;

      zone = &NODE_DATA(0)->node_zones[ZONE_NORMAL];
      cpu = get_cpu();
      pcp = per_cpu_ptr(zone->per_cpu_pageset, cpu);

      pindex = order_to_pindex(MIGRATE_UNMOVABLE, 0);
      head = &pcp->lists[pindex];
      count = 0;
      list_for_each_entry_safe(page, next, head, lru) { count++; }
      data.num_pages = count;

      put_cpu();

      if (copy_to_user((void __user *)arg, &data, sizeof(data))) {
        return -EFAULT;
      }
      break;
    }

    case RUBENCH_VA_TO_PA: {
      struct rubench_va_to_pa_data data;
      struct page *page = NULL;
      if (copy_from_user(&data, (void __user *)arg, sizeof(data))) {
        return -EFAULT;
      }

      get_user_pages_fast((unsigned long)data.va, 1, 0, &page);
      if (!page) {
        return -EFAULT;
      }
      data.pa = page_to_phys(page);
      put_page(page);

      if (copy_to_user((void __user *)arg, &data, sizeof(data))) {
        return -EFAULT;
      }
      break;
    }

    case RUBENCH_READ_PHYS: {
      struct rubench_read_phys_data data;
      void *virt_addr;
      if (copy_from_user(&data, (void __user *)arg, sizeof(data))) {
        return -EFAULT;
      }

      virt_addr = phys_to_virt(data.pa);
      if (!virt_addr) {
        return -EINVAL;
      }
      data.data = *(unsigned long *)virt_addr;

      if (copy_to_user((void __user *)arg, &data, sizeof(data))) {
        return -EFAULT;
      }
      break;
    }

    default:
      return -ENOTTY;
  }

  return 0;
}

static int __init rubench_init(void) {
  rubench_major = register_chrdev(0, DEVICE_NAME, &rubench_fops);
  if (rubench_major < 0) {
    pr_err("Rubench module registration failed\n");
    return rubench_major;
  }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
  rubench_class = class_create(CLASS_NAME);
#else
  rubench_class = class_create(THIS_MODULE, CLASS_NAME);
#endif
  if (IS_ERR(rubench_class)) {
    unregister_chrdev(rubench_major, DEVICE_NAME);
    pr_err("Failed to create device class\n");
    return PTR_ERR(rubench_class);
  }

  rubench_device = device_create(rubench_class, NULL, MKDEV(rubench_major, 0),
                                 NULL, DEVICE_NAME);
  if (IS_ERR(rubench_device)) {
    class_destroy(rubench_class);
    unregister_chrdev(rubench_major, DEVICE_NAME);
    pr_err("Failed to create device\n");
    return PTR_ERR(rubench_device);
  }

  pr_info("Rubench module loaded with major number %d\n", rubench_major);
  return 0;
}

static void __exit rubench_exit(void) {
  device_destroy(rubench_class, MKDEV(rubench_major, 0));
  class_destroy(rubench_class);
  unregister_chrdev(rubench_major, DEVICE_NAME);
  pr_info("Rubench module unloaded\n");
}

module_init(rubench_init);
module_exit(rubench_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Matej Bölcskei");