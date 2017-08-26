// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <limits.h>
#include <string>
#include <sys/stat.h>
#include <vector>

#include "common/CommonTypes.h"
#include "common/File.h"
#include "common/FileUtil.h"

#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef S_ISDIR
#define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
#endif

// This namespace has various generic functions related to files and paths.
// The code still needs a ton of cleanup.
// REMEMBER: strdup considered harmful!
namespace File
{
#ifdef _WIN32
FileInfo::FileInfo(const std::string& path)
{
  m_exists = _tstat64(UTF8ToTStr(path).c_str(), &m_stat) == 0;
}

FileInfo::FileInfo(const char* path) : FileInfo(std::string(path))
{
}
#else
FileInfo::FileInfo(const std::string& path) : FileInfo(path.c_str())
{
}

FileInfo::FileInfo(const char* path)
{
  m_exists = stat(path, &m_stat) == 0;
}
#endif

FileInfo::FileInfo(int fd)
{
  m_exists = fstat(fd, &m_stat);
}

bool FileInfo::Exists() const
{
  return m_exists;
}

bool FileInfo::IsDirectory() const
{
  return m_exists ? S_ISDIR(m_stat.st_mode) : false;
}

bool FileInfo::IsFile() const
{
  return m_exists ? !S_ISDIR(m_stat.st_mode) : false;
}

u64 FileInfo::GetSize() const
{
  return IsFile() ? m_stat.st_size : 0;
}

// Returns true if the path exists
bool Exists(const std::string& path)
{
  return FileInfo(path).Exists();
}

// Returns true if the path exists and is a directory
bool IsDirectory(const std::string& path)
{
  return FileInfo(path).IsDirectory();
}

// Returns true if the path exists and is a file
bool IsFile(const std::string& path)
{
  return FileInfo(path).IsFile();
}

// Deletes a given filename, return true on success
// Doesn't supports deleting a directory
bool Delete(const std::string& filename)
{
  const FileInfo file_info(filename);

  // Return true because we care about the file no
  // being there, not the actual delete.
  if (!file_info.Exists())
  {
    return true;
  }

  // We can't delete a directory
  if (file_info.IsDirectory())
  {
    return false;
  }

  if (unlink(filename.c_str()) == -1)
  {
    return false;
  }

  return true;
}

// Returns true if successful, or path already exists.
bool CreateDir(const std::string& path)
{
  if (mkdir(path.c_str(), 0755) == 0)
    return true;

  int err = errno;

  if (err == EEXIST)
  {
    return true;
  }

  return false;
}

// Creates the full path of fullPath returns true on success
bool CreateFullPath(const std::string& fullPath)
{
  int panicCounter = 100;

  if (Exists(fullPath))
  {
    return true;
  }

  size_t position = 0;
  while (true)
  {
    // Find next sub path
    position = fullPath.find(DIR_SEP_CHR, position);

    // we're done, yay!
    if (position == fullPath.npos)
      return true;

    // Include the '/' so the first call is CreateDir("/") rather than CreateDir("")
    std::string const subPath(fullPath.substr(0, position + 1));
    if (!IsDirectory(subPath))
      File::CreateDir(subPath);

    // A safety check
    panicCounter--;
    if (panicCounter <= 0)
    {
      return false;
    }
    position++;
  }
}

// Deletes a directory filename, returns true on success
bool DeleteDir(const std::string& filename)
{
  // check if a directory
  if (!IsDirectory(filename))
  {
    return false;
  }

  if (rmdir(filename.c_str()) == 0)
    return true;

  return false;
}

// renames file srcFilename to destFilename, returns true on success
bool Rename(const std::string& srcFilename, const std::string& destFilename)
{
  if (rename(srcFilename.c_str(), destFilename.c_str()) == 0)
    return true;
  return false;
}

static void FSyncPath(const char* path)
{
  int fd = open(path, O_RDONLY);
  if (fd != -1)
  {
    fsync(fd);
    close(fd);
  }
}

bool RenameSync(const std::string& srcFilename, const std::string& destFilename)
{
  if (!Rename(srcFilename, destFilename))
    return false;
  char* path = strdup(srcFilename.c_str());
  FSyncPath(path);
  FSyncPath(dirname(path));
  free(path);
  path = strdup(destFilename.c_str());
  FSyncPath(dirname(path));
  free(path);
  return true;
}

// copies file source_path to destination_path, returns true on success
bool Copy(const std::string& source_path, const std::string& destination_path)
{
  std::ifstream source{source_path, std::ios::binary};
  std::ofstream destination{destination_path, std::ios::binary};
  destination << source.rdbuf();
  return source.good() && destination.good();
}

// Returns the size of a file (or returns 0 if the path isn't a file that exists)
u64 GetSize(const std::string& path)
{
  return FileInfo(path).GetSize();
}

// Overloaded GetSize, accepts file descriptor
u64 GetSize(const int fd)
{
  return FileInfo(fd).GetSize();
}

// Overloaded GetSize, accepts FILE*
u64 GetSize(FILE* f)
{
  // can't use off_t here because it can be 32-bit
  u64 pos = ftello(f);
  if (fseeko(f, 0, SEEK_END) != 0)
  {
    return 0;
  }

  u64 size = ftello(f);
  if ((size != pos) && (fseeko(f, pos, SEEK_SET) != 0))
  {
    return 0;
  }

  return size;
}

// creates an empty file filename, returns true on success
bool CreateEmptyFile(const std::string& filename)
{
  if (!File::IOFile(filename, "wb"))
  {
    return false;
  }

  return true;
}

// Recursive or non-recursive list of files and directories under directory.
FSTEntry ScanDirectoryTree(const std::string& directory, bool recursive)
{
  FSTEntry parent_entry;
  parent_entry.physicalName = directory;
  parent_entry.isDirectory = true;
  parent_entry.size = 0;
#ifdef _WIN32
  // Find the first file in the directory.
  WIN32_FIND_DATA ffd;

  HANDLE hFind = FindFirstFile(UTF8ToTStr(directory + "\\*").c_str(), &ffd);
  if (hFind == INVALID_HANDLE_VALUE)
  {
    FindClose(hFind);
    return parent_entry;
  }
  // Windows loop
  do
  {
    const std::string virtual_name(TStrToUTF8(ffd.cFileName));
#else
  DIR* dirp = opendir(directory.c_str());
  if (!dirp)
    return parent_entry;

  // non Windows loop
  while (dirent* result = readdir(dirp))
  {
    const std::string virtual_name(result->d_name);
#endif
    if (virtual_name == "." || virtual_name == "..")
      continue;
    auto physical_name = directory + DIR_SEP + virtual_name;
    FSTEntry entry;
    const FileInfo file_info(physical_name);
    entry.isDirectory = file_info.IsDirectory();
    if (entry.isDirectory)
    {
      if (recursive)
        entry = ScanDirectoryTree(physical_name, true);
      else
        entry.size = 0;
      parent_entry.size += entry.size;
    }
    else
    {
      entry.size = file_info.GetSize();
    }
    entry.virtualName = virtual_name;
    entry.physicalName = physical_name;

    ++parent_entry.size;
    // Push into the tree
    parent_entry.children.push_back(entry);
#ifdef _WIN32
  } while (FindNextFile(hFind, &ffd) != 0);
  FindClose(hFind);
#else
  }
  closedir(dirp);
#endif

  return parent_entry;
}

// Deletes the given directory and anything under it. Returns true on success.
bool DeleteDirRecursively(const std::string& directory)
{
  bool success = true;

#ifdef _WIN32
  // Find the first file in the directory.
  WIN32_FIND_DATA ffd;
  HANDLE hFind = FindFirstFile(UTF8ToTStr(directory + "\\*").c_str(), &ffd);

  if (hFind == INVALID_HANDLE_VALUE)
  {
    FindClose(hFind);
    return false;
  }

  // Windows loop
  do
  {
    const std::string virtualName(TStrToUTF8(ffd.cFileName));
#else
  DIR* dirp = opendir(directory.c_str());
  if (!dirp)
    return false;

  // non Windows loop
  while (dirent* result = readdir(dirp))
  {
    const std::string virtualName = result->d_name;
#endif

    // check for "." and ".."
    if (((virtualName[0] == '.') && (virtualName[1] == '\0')) ||
        ((virtualName[0] == '.') && (virtualName[1] == '.') && (virtualName[2] == '\0')))
      continue;

    std::string newPath = directory + DIR_SEP_CHR + virtualName;
    if (IsDirectory(newPath))
    {
      if (!DeleteDirRecursively(newPath))
      {
        success = false;
        break;
      }
    }
    else
    {
      if (!File::Delete(newPath))
      {
        success = false;
        break;
      }
    }

#ifdef _WIN32
  } while (FindNextFile(hFind, &ffd) != 0);
  FindClose(hFind);
#else
  }
  closedir(dirp);
#endif
  if (success)
    File::DeleteDir(directory);

  return success;
}

// Create directory and copy contents (does not overwrite existing files)
void CopyDir(const std::string& source_path, const std::string& dest_path, bool destructive)
{
  if (source_path == dest_path)
    return;
  if (!Exists(source_path))
    return;
  if (!Exists(dest_path))
    File::CreateFullPath(dest_path);

#ifdef _WIN32
  WIN32_FIND_DATA ffd;
  HANDLE hFind = FindFirstFile(UTF8ToTStr(source_path + "\\*").c_str(), &ffd);

  if (hFind == INVALID_HANDLE_VALUE)
  {
    FindClose(hFind);
    return;
  }

  do
  {
    const std::string virtualName(TStrToUTF8(ffd.cFileName));
#else
  DIR* dirp = opendir(source_path.c_str());
  if (!dirp)
    return;

  while (dirent* result = readdir(dirp))
  {
    const std::string virtualName(result->d_name);
#endif
    // check for "." and ".."
    if (virtualName == "." || virtualName == "..")
      continue;

    std::string source = source_path + DIR_SEP + virtualName;
    std::string dest = dest_path + DIR_SEP + virtualName;
    if (IsDirectory(source))
    {
      if (!Exists(dest))
        File::CreateFullPath(dest + DIR_SEP);
      CopyDir(source, dest, destructive);
    }
    else if (!Exists(dest) && !destructive)
    {
      Copy(source, dest);
    }
    else
    {
      Rename(source, dest);
    }
#ifdef _WIN32
  } while (FindNextFile(hFind, &ffd) != 0);
  FindClose(hFind);
#else
  }
  closedir(dirp);
#endif
}

std::string CreateTempDir()
{
#ifdef _WIN32
  TCHAR temp[MAX_PATH];
  if (!GetTempPath(MAX_PATH, temp))
    return "";

  GUID guid;
  CoCreateGuid(&guid);
  TCHAR tguid[40];
  StringFromGUID2(guid, tguid, 39);
  tguid[39] = 0;
  std::string dir = TStrToUTF8(temp) + "/" + TStrToUTF8(tguid);
  if (!CreateDir(dir))
    return "";
  dir = ReplaceAll(dir, "\\", DIR_SEP);
  return dir;
#else
  const char* base = getenv("TMPDIR") ?: "/tmp";
  std::string path = std::string(base) + "/DolphinWii.XXXXXX";
  if (!mkdtemp(&path[0]))
    return "";
  return path;
#endif
}

std::string GetTempFilenameForAtomicWrite(const std::string& path)
{
  std::string abs = path;
#ifdef _WIN32
  TCHAR absbuf[MAX_PATH];
  if (_tfullpath(absbuf, UTF8ToTStr(path).c_str(), MAX_PATH) != nullptr)
    abs = TStrToUTF8(absbuf);
#else
  char absbuf[PATH_MAX];
  if (realpath(path.c_str(), absbuf) != nullptr)
    abs = absbuf;
#endif
  return abs + ".xxx";
}

bool WriteStringToFile(const std::string& str, const std::string& filename)
{
  return File::IOFile(filename, "wb").WriteBytes(str.data(), str.size());
}

bool ReadFileToString(const std::string& filename, std::string& str)
{
  File::IOFile file(filename, "rb");
  auto const f = file.GetHandle();

  if (!f)
    return false;

  size_t read_size;
  str.resize(GetSize(f));
  bool retval = file.ReadArray(&str[0], str.size(), &read_size);

  return retval;
}

}  // namespace
