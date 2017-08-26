// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <cstdlib>
#include <string>
#include <vector>

#include "common/CommonTypes.h"

namespace File
{
class IOFile;
}

class BootExecutableReader
{
public:
  explicit BootExecutableReader(const std::string& file_name);
  explicit BootExecutableReader(File::IOFile file);
  explicit BootExecutableReader(const std::vector<u8>& buffer);
  virtual ~BootExecutableReader();

  virtual u32 GetEntryPoint() const = 0;
  virtual bool IsValid() const = 0;
  virtual bool LoadIntoMemory() const = 0;

protected:
  std::vector<u8> m_bytes;
};
