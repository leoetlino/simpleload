// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <iomanip>
#include <ostream>
#include <sstream>

#include "StringUtil.h"

std::string ArrayToString(const u8* data, u32 size, int line_len, bool spaces)
{
  std::ostringstream oss;
  oss << std::setfill('0') << std::hex;

  for (int line = 0; size; ++data, --size)
  {
    oss << std::setw(2) << (int)*data;

    if (line_len == ++line)
    {
      oss << '\n';
      line = 0;
    }
    else if (spaces)
      oss << ' ';
  }

  return oss.str();
}
