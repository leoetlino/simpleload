// TinyLoad - a simple region free (original) game launcher in 4k
// This code is licensed to you under the terms of the GNU GPL, version 2;
// see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
// Copyright 2008-2009  Hector Martin  <marcan@marcansoft.com>

#pragma once

#include <array>

#include "common/CommonTypes.h"

namespace DI
{
struct PartitionTableInfo
{
  u32 count;
  u32 offset;
  std::array<u32, 6> pad;
};

struct Partition
{
  u32 offset;
  u32 type;
};

using PartitionTable = std::array<Partition, 32>;

int Init();
int GetCoverStatus();
int RequestError();
int Read(void* dst, u32 size, u32 offset);
int UnencryptedRead(void* dst, u32 size, u32 offset);
int Identify();
int Reset();
int OpenPartition(u32 offset, u8* tmd);
int ClosePartition();
using DiscId = std::array<u32, 8>;
int ReadDiscId(DiscId* disc_id);
int Shutdown();
}  // namespace DI
