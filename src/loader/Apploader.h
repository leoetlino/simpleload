#pragma once

#include <array>
#include <type_traits>

#include "common/CommonTypes.h"
#include "loader/Boot.h"

namespace Apploader
{
enum class Status : u32
{
  Loaded = 0,
  CallAgain = 1,
};

using ReportFunction = std::add_pointer_t<void(const char* format, ...)>;

using InitFunction = std::add_pointer_t<void(ReportFunction report_function)>;
using MainFunction = std::add_pointer_t<Status(void** read_dest, u32* read_size, u32* read_offset)>;
using FinalFunction = std::add_pointer_t<Boot::GameEntryFunction()>;
using EntryFunction =
    std::add_pointer_t<void(InitFunction* init, MainFunction* main, FinalFunction* final)>;

struct Header
{
  std::array<char, 16> revision;
  EntryFunction entry;
  s32 size;
  s32 trailersize;
  s32 padding;
};
}  // namespace Apploader
