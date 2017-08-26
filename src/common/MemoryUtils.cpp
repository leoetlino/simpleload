#include "common/MemoryUtils.h"

namespace Common
{
// this saves 12 bytes
static void Sync() __attribute__((noinline));
static void Sync()
{
  asm("sync ; isync");
}

void SyncBeforeRead(void* p, u32 len)
{
  uintptr_t a = (uintptr_t)p & ~0x1f;
  uintptr_t b = ((uintptr_t)p + len + 0x1f) & ~0x1f;

  for (; a < b; a += 32)
    asm("dcbi 0,%0" : : "b"(a) : "memory");

  Sync();
}

void SyncAfterWrite(const void* p, u32 len)
{
  uintptr_t a = (uintptr_t)p & ~0x1f;
  uintptr_t b = ((uintptr_t)p + len + 0x1f) & ~0x1f;

  for (; a < b; a += 32)
    asm("dcbf 0,%0" : : "b"(a));

  Sync();
}

void SyncBeforeExec(const void* p, u32 len)
{
  uintptr_t a = (uintptr_t)p & ~0x1f;
  uintptr_t b = ((uintptr_t)p + len + 0x1f) & ~0x1f;

  for (; a < b; a += 32)
    asm("dcbst 0,%0 ; sync ; icbi 0,%0" : : "b"(a));

  Sync();
}
}  // namespace Common
