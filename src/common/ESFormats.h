// Copyright 2017 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <cstddef>

#include "common/CommonTypes.h"

namespace IOS
{
namespace ES
{
enum TitleFlags : u32
{
  // All official titles have this flag set.
  TITLE_TYPE_DEFAULT = 0x1,
  // Unknown.
  TITLE_TYPE_0x4 = 0x4,
  // Used for DLC titles.
  TITLE_TYPE_DATA = 0x8,
  // Unknown.
  TITLE_TYPE_0x10 = 0x10,
  // Appears to be used for WFS titles.
  TITLE_TYPE_WFS_MAYBE = 0x20,
  // Unknown.
  TITLE_TYPE_CT = 0x40,
};

#pragma pack(push, 4)
enum class SignatureType : u32
{
  RSA4096 = 0x00010000,
  RSA2048 = 0x00010001,
};

struct SignatureRSA2048
{
  SignatureType type;
  u8 sig[0x100];
  u8 fill[0x3c];
  char issuer[0x40];
};
static_assert(sizeof(SignatureRSA2048) == 0x180, "Wrong size for SignatureRSA2048");

struct Content
{
  bool IsShared() const { return (type & 0x8000) != 0; }
  bool IsOptional() const { return (type & 0x4000) != 0; }
  u32 id;
  u16 index;
  u16 type;
  u64 size;
  std::array<u8, 20> sha1;
};
static_assert(sizeof(Content) == 36, "Content has the wrong size");

constexpr size_t MAX_POSSIBLE_TMD_SIZE = 0x49e4;
using TMDBytes = std::array<u8, MAX_POSSIBLE_TMD_SIZE>;

struct TMDHeader
{
  SignatureRSA2048 signature;
  u8 tmd_version;
  u8 ca_crl_version;
  u8 signer_crl_version;
  u64 ios_id;
  u64 title_id;
  u32 title_flags;
  u16 group_id;
  u16 zero;
  u16 region;
  u8 ratings[16];
  u8 reserved[12];
  u8 ipc_mask[12];
  u8 reserved2[18];
  u32 access_rights;
  u16 title_version;
  u16 num_contents;
  u16 boot_index;
  u16 fill2;
};
static_assert(sizeof(TMDHeader) == 0x1e4, "TMDHeader has the wrong size");
#pragma pack(pop)
}  // namespace ES
}  // namespace IOS
