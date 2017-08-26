// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include "loader/executable_readers/Reader.h"

#include "common/File.h"

BootExecutableReader::BootExecutableReader(const std::string& file_name)
    : BootExecutableReader(File::IOFile{file_name, "rb"})
{
}

BootExecutableReader::BootExecutableReader(File::IOFile file)
{
  file.Seek(0, SEEK_SET);
  m_bytes.resize(file.GetSize());
  file.ReadBytes(m_bytes.data(), m_bytes.size());
}

BootExecutableReader::BootExecutableReader(const std::vector<u8>& bytes) : m_bytes(bytes)
{
}

BootExecutableReader::~BootExecutableReader() = default;
