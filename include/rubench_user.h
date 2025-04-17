/*
 * Copyright (C) 2025 Matej Bölcskei, ETH Zurich
 * Licensed under the GNU General Public License as published by the Free Software Foundation, version 3.
 * See LICENSE or <https://www.gnu.org/licenses/gpl-3.0.html> for details.
 * 
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

void rubench_open();
void rubench_close();

int rubench_get_blocks();
unsigned long rubench_va_to_pa(void *va);
unsigned long rubench_read_phys(unsigned long pa);