/*
 *   Copyright 2016 Matt Edlefsen <matt.edlefsen@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, Inc., 53 Temple Place Ste 330,
 *   Boston MA 02111-1307, USA; either version 2 of the License, or
 *   (at your option) any later version; incorporated herein by reference.
 *
 * ----------------------------------------------------------------------- */

#include <string.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <gpt.h>

class silence
{
public:
  silence() : orig(std::cout.rdbuf()), output() {
    std::cout.rdbuf(&output);
  }

  ~silence() {
    std::cout.rdbuf(orig);
  }
private:
  std::streambuf* orig;
  std::stringbuf output;
};

class lgpt : public GPTData {
public:
  lgpt(const char* filename) : GPTData() {
    LoadPartitions(filename);
  }
  GPTPart& part(int i) {
    return partitions[i];
  }
private:
  silence s;
};

extern "C" {

#include "disks.h"

int create_gpt_table(const char* filename)
{
  try
  {
    lgpt data(filename);
    data.ClearGPTData();
    data.SaveGPTData(1);
  } catch(...) {
    return 1;
  }
  return 0;
}

int create_partition(const char* filename, int part_num, uint64_t start, uint64_t size, const char* label)
{
  try
  {
    lgpt data(filename);
    if(part_num == 0) {
      part_num = data.FindFirstFreePart();
    } else {
      --part_num;
    }
    if(data.CheckHeaderValidity() < 3 || !data.ValidPartNum(part_num) || data.IsUsedPartNum(part_num)) return 2;
    if(start == 0)
    {
      if(data.CountParts()) {
        uint32_t first, last;
        data.GetPartRange(&first,&last);
        start = data.FindFirstAvailable(data[last].GetLastLBA());
      } else
      {
        start = data.GetFirstUsableLBA();
      }
      data.Align(&start);
    }
    uint64_t end = start + size;
    if(size == 0)
    {
      end = data.GetLastUsableLBA();
      data.Align(&end);
    }
    if(!data.CreatePartition(part_num, start, end)) return 3;
    if(label)
    {
      data.SetName(part_num, std::string(label));
    }
    data.SaveGPTData(1);
  } catch(...)
  {
    return 1;
  }
  return 0;
}

int set_partition_bootable(const char* filename, int part_num, bool value)
{
  try
  {
    --part_num;
    lgpt data(filename);
    if(data.CheckHeaderValidity() < 3 || !data.IsUsedPartNum(part_num)) return 2;
    data.ManageAttributes(part_num, std::string(value ? "set" : "clear"), std::string("2"));

    data.SaveGPTData(1);
  } catch(...)
  {
    return 1;
  }
  return 0;
}

int collect_disk_info(disk_info* di, const char* filename)
{
  try
  {
    lgpt data(filename);
    if(data.CheckHeaderValidity() < 3) return 2;
    di->num_partitions = data.CountParts();
    if(di->num_partitions == 0) {
      di->partitions = NULL;
      return 0;
    }
    di->partitions = new part_info[di->num_partitions];
    memset(di->partitions,0,sizeof(*di->partitions));
    uint32_t start, end;
    data.GetPartRange(&start, &end);
    int pi = 0;
    for(uint32_t i = start; i <= end; ++i)
    {
      if(data.IsUsedPartNum(i))
      {
        GPTPart& p = data.part(i);
        di->partitions[pi].number = i+1;
        di->partitions[pi].bootable = p.GetAttributes().GetAttributes() & 4;
        di->partitions[pi].start = (uint64_t)p.GetFirstLBA();
        di->partitions[pi].size = (uint64_t)p.GetLengthLBA();
        std::string label = p.GetDescription();
        char* clabel = new char[label.size() + 1];
        strncpy(clabel, label.c_str(), label.size() + 1);
        di->partitions[pi].label = clabel;
        ++pi;
      }
    }
  } catch(...)
  {
    return 1;
  }
  return 0;
}

void print_disk_info(disk_info* di) {
  for(int i = 0; i < di->num_partitions; ++i)
  {
    printf("%d\t%llu\t%llu\t%s\t%s\n",
           di->partitions[i].number,
           di->partitions[i].start,
           di->partitions[i].size,
           di->partitions[i].bootable ? "true" : "false",
           di->partitions[i].label
    );
  }
}

int free_disk_info(disk_info* di) {
  for(int i = 0; i < di->num_partitions; ++i)
    delete [] di->partitions[i].label;
  delete [] di->partitions;
  di->num_partitions = 0;
  di->partitions = 0;
  return 0;
}

}
