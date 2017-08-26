#include "loader/Boot.h"

#include <array>
#include <cinttypes>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <fat.h>
#include <ogc/ios.h>
#include <ogc/ipc.h>
#include <ogc/system.h>

#include "common/ESFormats.h"
#include "common/File.h"
#include "common/MemoryUtils.h"
#include "loader/Apploader.h"
#include "loader/executable_readers/ElfReader.h"

namespace Boot
{
static DI::Partition* FindDataPartition(const DI::PartitionTableInfo& info,
                                        DI::PartitionTable& partitions)
{
  for (std::size_t i = 0; i < info.count; ++i)
  {
    if (partitions[i].type == 0)
      return &partitions[i];
  }
  return nullptr;
}

std::optional<DI::Partition> InitAndOpenPartition(bool reload_ios)
{
  auto fail = [](const char* message) {
    printf("InitAndOpenPartition: %s\n", message);
    return std::optional<DI::Partition>{};
  };

  if (DI::Init() < 0)
    return fail("DI::Init failed");

  if (DI::GetCoverStatus() != 2)
    return fail("DI::GetCoverStatus failed");

  if (DI::Reset() != 1)
    return fail("DI::Reset failed");

  if (DI::Identify() != 1)
    return fail("DI::Identify failed");

  alignas(32) static DI::DiscId disc_id{};
  if (DI::ReadDiscId(&disc_id) != 1)
    return fail("DI::ReadDiscId failed");

  if (disc_id[6] != 0x5d1c9ea3)
    return fail("Not a Wii disc");

  alignas(32) static DI::PartitionTableInfo part_table_info{};
  if (DI::UnencryptedRead(&part_table_info, sizeof(part_table_info), 0x10000) != 1)
    return fail("Failed to read partition table info");

  alignas(32) static DI::PartitionTable partitions{};
  if (DI::UnencryptedRead(partitions.data(), sizeof(partitions), part_table_info.offset) != 1)
    return fail("Failed to read partition table");

  DI::Partition* partition = FindDataPartition(part_table_info, partitions);
  if (!partition)
    return fail("Failed to find data partition");

  alignas(64) static IOS::ES::TMDBytes tmd{};
  if (DI::OpenPartition(partition->offset, tmd.data()) != 1)
    return fail("Failed to open data partition");

  if (reload_ios)
  {
    const u64 ios_title_id = Common::Read<u64>(&tmd[offsetof(IOS::ES::TMDHeader, ios_id)]);
    if (IOS_ReloadIOS(static_cast<u32>(ios_title_id)) != IPC_OK)
      return fail("Failed to reload IOS");
  }

  return *partition;
}

static void OSReport(const char* format, ...)
{
  printf("[OSReport] ");
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

static GameEntryFunction LoadBinaryFromSD(const std::array<u8, 6>& game_id_bytes)
{
  auto fail = [](const char* message) {
    printf("LoadBinaryFromSD: %s\n", message);
    return nullptr;
  };

  const std::string game_id{game_id_bytes.cbegin(), game_id_bytes.cend()};
  printf("LoadBinaryFromSD: Looking for a binary for %s\n", game_id.c_str());

  if (!fatInitDefault())
    return fail("fatInitDefault failed");

  ElfReader elf_reader{"/" + game_id + ".elf"};
  if (!elf_reader.IsValid())
    return fail("Not a valid ELF");

  if (!elf_reader.LoadIntoMemory())
    return fail("Failed to load ELF to memory");

  return reinterpret_cast<GameEntryFunction>(elf_reader.GetEntryPoint());
}

GameEntryFunction InitApploader(bool load_custom_binary_from_sd)
{
  auto fail = [](const char* message) {
    printf("InitApploader: %s\n", message);
    return nullptr;
  };

  if (DI::Read(reinterpret_cast<void*>(0x80000000), 0x20, 0) != 1)
    return fail("Failed to read partition header");

  alignas(32) static Apploader::Header header{};
  if (DI::Read(&header, 0x20, 0x910) != 1)
    return fail("Failed to read apploader header");

  void* const apploader_dest = reinterpret_cast<void*>(0x81200000);
  const u32 apploader_size = header.size + header.trailersize;

  printf("Loading apploader (%" PRIu32 " bytes)...\n", apploader_size);
  if (DI::Read(apploader_dest, apploader_size, 0x918) != 1)
    return fail("Failed to read apploader");

  Common::SyncBeforeExec(apploader_dest, apploader_size);

  // Initialise apploader functions.
  Apploader::InitFunction init_function = nullptr;
  Apploader::MainFunction main_function = nullptr;
  Apploader::FinalFunction final_function = nullptr;
  printf("Calling apploader entry function (%p)\n", header.entry);
  header.entry(&init_function, &main_function, &final_function);

  printf("Calling apploader init function (%p)\n", init_function);
  init_function(OSReport);

  printf("Calling apploader main function (%p)\n", main_function);
  {
    void* dest = nullptr;
    u32 size = 0;
    u32 offset = 0;

    while (main_function(&dest, &size, &offset) == Apploader::Status::CallAgain)
    {
      printf("  read %" PRIu32 " bytes from %08" PRIx32 " to %p\n", size, offset, dest);
      DI::Read(dest, size, offset);
    }
  }

  printf("Calling apploader final function (%p)\n", final_function);
  const GameEntryFunction entry_point = final_function();

  if (!load_custom_binary_from_sd)
    return entry_point;

  std::array<u8, 6> game_id;
  if (DI::Read(game_id.data(), game_id.size(), 0) != 1)
    return fail("Failed to read game ID");

  const GameEntryFunction sd_entry_point = LoadBinaryFromSD(game_id);
  if (!sd_entry_point)
    return entry_point;

  printf("Returning SD entry point\n");
  return sd_entry_point;
}

static u32 Read_U32(uintptr_t address)
{
  return *(reinterpret_cast<u32*>(address));
}

static void Write_U32(u32 value, uintptr_t address)
{
  *(reinterpret_cast<u32*>(address)) = value;
}

// Adapted from Dolphin (GPLv2+)
void PokeConstants(const DI::Partition& game_partition)
{
  Write_U32(0x0D15EA5E, 0x80000020);  // Another magic word
  Write_U32(0x00000001, 0x80000024);  // Unknown
  Write_U32(0x01800000, 0x80000028);  // MEM1 size 24MB
  Write_U32(0x00000023, 0x8000002c);  // Production Board Model
  Write_U32(0x00000000, 0x80000030);  // Init
  Write_U32(0x817FEC60, 0x80000034);  // Init
  Write_U32(0x8008f7b8, 0x800000e4);  // Thread Init
  Write_U32(0x01800000, 0x800000f0);  // "Simulated memory size" (debug mode?)
  Write_U32(0x8179b500, 0x800000f4);  // __start
  Write_U32(0x0e7be2c0, 0x800000f8);  // Bus speed
  Write_U32(0x2B73A840, 0x800000fc);  // CPU speed
  Write_U32(0x00000000, 0x800030c0);  // EXI
  Write_U32(0x00000000, 0x800030c4);  // EXI
  Write_U32(0x00000000, 0x800030dc);  // Time
  Write_U32(0xffffffff, 0x800030d8);  // Unknown, set by any official NAND title
  Write_U32(0x00000000, 0x800030e0);
  Write_U32(0x00000000, 0x800030e4);
  Write_U32(0x00000000, 0x800030f0);  // Apploader
  Write_U32(0x80000113, 0x8000315c);
  Write_U32(Read_U32(0x80000000), 0x80003180);

  // While reading a disc, the system menu reads the first partition table
  // (0x20 bytes from 0x00040020) and stores a pointer to the data partition entry.
  // When launching the disc game, it copies the partition type and offset to 0x3194
  // and 0x3198 respectively.
  Write_U32(game_partition.type, 0x80003194);
  Write_U32(game_partition.offset, 0x80003198);

  *(volatile u32*)0xCD006C00 = 0x00000000;  // deinit audio due to libogc fail
  Common::SyncAfterWrite(reinterpret_cast<void*>(0x80000000), 0x3f00);
}

bool CleanupBeforeLaunch()
{
  auto fail = [](const char* message) {
    printf("CleanupBeforeLaunch: %s\n", message);
    return false;
  };

  if (DI::Shutdown() != IPC_OK)
    return fail("Failed to clean up DI");

  fatUnmount("sd:/");
  SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);
  return true;
}
}  // namespace Boot
