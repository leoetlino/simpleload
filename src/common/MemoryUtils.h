#pragma once

#include <cstdint>
#include <cstring>
#include <type_traits>

#include <ogc/system.h>

namespace Common
{
template <typename T>
constexpr T* UncachedPtr(T* pointer)
{
  return reinterpret_cast<T*>(std::uintptr_t(pointer) + (SYS_BASE_UNCACHED - SYS_BASE_CACHED));
}

template <typename T>
inline T Read(const u8* data)
{
  static_assert(std::is_arithmetic<T>::value, "function only makes sense with arithmetic types");

  T value;
  std::memcpy(&value, data, sizeof(T));
  return value;
}

void SyncBeforeRead(void* p, u32 len);
void SyncAfterWrite(const void* p, u32 len);
void SyncBeforeExec(const void* p, u32 len);
}  // namespace Common
