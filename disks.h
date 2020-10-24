/*
 *   Copyright 2016 Matt Edlefsen <matt@xforty.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, Inc., 53 Temple Place Ste 330,
 *   Boston MA 02111-1307, USA; either version 2 of the License, or
 *   (at your option) any later version; incorporated herein by reference.
 *
 * ----------------------------------------------------------------------- */

#ifndef DISKS_H
#define DISKS_H

typedef struct {
  int number;
  int bootable;
  uint64_t start;
  uint64_t size;
  const char* label;
} part_info;

typedef struct {
  part_info* partitions;
  int num_partitions;
} disk_info;

int create_gpt_table(const char* filename);
int create_partition(const char* filename, int part_num, uint64_t start, uint64_t size, const char* label);
int set_partition_bootable(const char* filename, int part_num, bool value);
int collect_disk_info(disk_info* di, const char* filename);
void print_disk_info(disk_info* di);
int free_disk_info(disk_info* di);

#endif
