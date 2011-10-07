#ifndef ZIPFILE_H
#define ZIPFILE_H

#include "common.h"

#define LOCAL_FILE_HEADER 0x04034b50
#define DIRECTORY_HEADER 0x06054b50
#define DIRECTORY_FILE_HEADER 0x02014b50


#pragma pack(2)

typedef struct
{
    u32 sig;
    u16 version;
    u16 flag;   // bit 3 = trailing data descriptor
    u16 compression;
    u16 modTime;
    u16 modDate;
    u32 crc32;
    u32 compressedSize;
    u32 uncompressedSize;
    u16 fileNameLen;
    u16 extraLen;
} LocalFileHeader;

typedef struct
{
    u32 sig;
    u16 nDisk;
    u16 nStartDisk;
    u16 nDirectoryEntries;
    u16 totalDirectoryEntries;
    u32 directorySize;
    u32 directoryOffset;
    u16 commentLen;
} DirectoryHeader;

typedef struct
{
  u32 sig;
  u16 versionCreatedBy;
  u16 version;
  u16 flag;
  u16 compression;
  u16 modTime;
  u16 modDate;
  u32 crc32;
  u32 compressedSize;
  u32 uncompressedSize;
  u16 fileNameLen;
  u16 extraLen;
  u16 commentLen;
  u16 diskStart;
  u16 internalAttributes;
  u32 externalAttributes;
  u32 headerOffset;
} DirectoryFileHeader;

#pragma pack()

#define MAX_ZIP_NAME 256
typedef struct
{
    DirectoryFileHeader info;
    string name;
} ZipFileInfo;

bool ZipInfo(FILE* file, vector<ZipFileInfo>& files);
u8* ZipDecompress(FILE* file, const ZipFileInfo& f);

#endif // ZIPFILE_H
