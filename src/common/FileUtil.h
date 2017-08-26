// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <cstddef>
#include <fstream>
#include <string>
#include <vector>

#include <sys/stat.h>

#include "common/CommonTypes.h"

namespace File
{
constexpr char DIR_SEP_CHR = '/';
const std::string DIR_SEP = "/";

// FileSystem tree node/
struct FSTEntry
{
  bool isDirectory;
  u64 size;                  // File length, or for directories, recursive count of children
  std::string physicalName;  // Name on disk
  std::string virtualName;   // Name in FST names table
  std::vector<FSTEntry> children;
};

// The functions in this class are functionally identical to the standalone functions
// below, but if you are going to be calling more than one of the functions using the
// same path, creating a single FileInfo object and calling its functions multiple
// times is faster than calling standalone functions multiple times.
class FileInfo final
{
public:
  explicit FileInfo(const std::string& path);
  explicit FileInfo(const char* path);
  explicit FileInfo(int fd);

  // Returns true if the path exists
  bool Exists() const;
  // Returns true if the path exists and is a directory
  bool IsDirectory() const;
  // Returns true if the path exists and is a file
  bool IsFile() const;
  // Returns the size of a file (or returns 0 if the path doesn't refer to a file)
  u64 GetSize() const;

private:
  struct stat m_stat;
  bool m_exists;
};

// Returns true if the path exists
bool Exists(const std::string& path);

// Returns true if the path exists and is a directory
bool IsDirectory(const std::string& path);

// Returns true if the path exists and is a file
bool IsFile(const std::string& path);

// Returns the size of a file (or returns 0 if the path isn't a file that exists)
u64 GetSize(const std::string& path);

// Overloaded GetSize, accepts file descriptor
u64 GetSize(const int fd);

// Overloaded GetSize, accepts FILE*
u64 GetSize(FILE* f);

// Returns true if successful, or path already exists.
bool CreateDir(const std::string& filename);

// Creates the full path of fullPath returns true on success
bool CreateFullPath(const std::string& fullPath);

// Deletes a given filename, return true on success
// Doesn't supports deleting a directory
bool Delete(const std::string& filename);

// Deletes a directory filename, returns true on success
bool DeleteDir(const std::string& filename);

// renames file srcFilename to destFilename, returns true on success
bool Rename(const std::string& srcFilename, const std::string& destFilename);

// ditto, but syncs the source file and, on Unix, syncs the directories after rename
bool RenameSync(const std::string& srcFilename, const std::string& destFilename);

// copies file srcFilename to destFilename, returns true on success
bool Copy(const std::string& srcFilename, const std::string& destFilename);

// creates an empty file filename, returns true on success
bool CreateEmptyFile(const std::string& filename);

// Recursive or non-recursive list of files and directories under directory.
FSTEntry ScanDirectoryTree(const std::string& directory, bool recursive);

// deletes the given directory and anything under it. Returns true on success.
bool DeleteDirRecursively(const std::string& directory);

// Create directory and copy contents (optionally overwrites existing files)
void CopyDir(const std::string& source_path, const std::string& dest_path,
             bool destructive = false);

// Creates and returns the path to a new temporary directory.
std::string CreateTempDir();

// Get a filename that can hopefully be atomically renamed to the given path.
std::string GetTempFilenameForAtomicWrite(const std::string& path);

bool WriteStringToFile(const std::string& str, const std::string& filename);
bool ReadFileToString(const std::string& filename, std::string& str);

// To deal with Windows being dumb at unicode:
template <typename T>
void OpenFStream(T& fstream, const std::string& filename, std::ios_base::openmode openmode)
{
  fstream.open(filename.c_str(), openmode);
}

}  // namespace
