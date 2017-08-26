// TinyLoad - a simple region free (original) game launcher in 4k
// This code is licensed to you under the terms of the GNU GPL, version 2;
// see file COPYING or http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt
// Copyright 2008-2009  Hector Martin  <marcan@marcansoft.com>

// For simplicity and because I'm lazy, modified to use libogc.
// As a result, we're not so tiny anymore, but less work.

#include "loader/DI.h"

#include <ogc/ipc.h>

static int di_fd = 0;
alignas(64) static u32 inbuf[16];
alignas(64) static u32 outbuf[16];

namespace DI
{
int Init()
{
  di_fd = IOS_Open("/dev/di", IPC_OPEN_NONE);
  return di_fd;
}

int Shutdown(void)
{
  return IOS_Close(di_fd);
}

static int ioctl_std(int num)
{
  inbuf[0x00] = num << 24;
  return IOS_Ioctl(di_fd, num, inbuf, 0x20, outbuf, 0x20);
}

int GetCoverStatus()
{
  int ret = ioctl_std(0x88);
  if (ret != 1)
    return -1;

  return outbuf[0];
}

int Identify()
{
  return ioctl_std(0x12);
}

int Read(void* dst, u32 size, u32 offset)
{
  inbuf[0x00] = 0x71000000;
  inbuf[0x01] = size;
  inbuf[0x02] = offset;

  return IOS_Ioctl(di_fd, 0x71, inbuf, 0x20, dst, size);
}

int UnencryptedRead(void* dst, u32 size, u32 offset)
{
  inbuf[0x00] = 0x8D000000;
  inbuf[0x01] = size;
  inbuf[0x02] = offset;

  return IOS_Ioctl(di_fd, 0x8D, inbuf, 0x20, dst, size);
}

int Reset(void)
{
  inbuf[0x01] = 1;

  return ioctl_std(0x8A);
}

int OpenPartition(u32 offset, u8* tmd)
{
  alignas(64) static ioctlv vectors[16];

  inbuf[0x00] = 0x8B000000;
  inbuf[0x01] = offset;

  vectors[0].data = inbuf;
  vectors[0].len = 0x20;
  vectors[1].data = nullptr;
  vectors[1].len = 0x2a4;
  vectors[2].data = nullptr;
  vectors[2].len = 0;

  vectors[3].data = tmd;
  vectors[3].len = 0x49e4;
  vectors[4].data = outbuf;
  vectors[4].len = 0x20;

  return IOS_Ioctlv(di_fd, 0x8B, 3, 2, vectors);
}

int ReadDiscId(DiscId* disc_id)
{
  inbuf[0x00] = 0x70000000;

  return IOS_Ioctl(di_fd, 0x70, inbuf, 0x20, disc_id->data(), 0x20);
}
}  // namespace DI
