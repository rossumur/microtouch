
/* Copyright (c) 2010, Peter Barrett  
**  
** Permission to use, copy, modify, and/or distribute this software for  
** any purpose with or without fee is hereby granted, provided that the  
** above copyright notice and this permission notice appear in all copies.  
**  
** THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL  
** WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED  
** WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR  
** BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES  
** OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,  
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS  
** SOFTWARE.  
*/

#include "Platform.h"
	
// Fix these for Big endian or architectures that don't like misaligned accesses
u16 GET16(const u8* p)
{
	return *((u16*)p);
}

u32 GET32(const u8* p)
{
	return *((u32*)p);
}

static struct
{
	u32 rootStart;
	u16 rootCount;
	u16 sectorsPerCluster;

	u32 fatStart;
	u32 fatCount;

	u32 clusterStart;
	u32 rootCluster;

	u8* buffer;
	ReadProc readProc;
} _fat;

void LoadExtents(Extent* extents, int maxExtents);

u8 FAT_Init(u8* buf, ReadProc readProc)
{
	_fat.buffer = buf;
	_fat.readProc = readProc;

	if (_fat.readProc(buf, 0) != 0)
		return 0;

	if (buf[510] != 0x55 || buf[511] != 0xAA)
	{
		// USB drives don't have partitions
	}

	u8 partitionType = buf[450];
	u32 bootSector = GET32(buf + 454);
	bool valid = false;
	switch (partitionType)
	{
		case 0x00:
			break;

		case 0x0B:	// FAT 32
		case 0x0C:	// FAT 32
		case 0x0E:
		case 0x0F:
			valid = true;
			break;

		default:
			valid = partitionType < 7;
	}

	if (!valid)
		bootSector = 0;	// might not have a partition at start of disk (USB drives etc)

	if (_fat.readProc(_fat.buffer, bootSector) != 0)
		return 0;

	if (GET16(buf + 11) != 512)	// bytes per sector
		return 0;

	_fat.sectorsPerCluster = buf[13];
	u16 reservedSectors = GET16(buf + 14);
	u16 numFats = buf[16];
	u16 rootDirectectoryCount = GET16(buf+17);
	_fat.fatCount = GET16(buf+22);

	if (_fat.fatCount == 0)
	{
		_fat.fatCount = GET32(buf+36);	// is FAT32
		_fat.rootCluster = GET32(buf+44);
	} else
		_fat.rootCluster = 0;	// Indicates FAT16

	_fat.fatStart = bootSector + reservedSectors;
	_fat.rootStart = _fat.fatStart + _fat.fatCount*numFats;
	_fat.rootCount = rootDirectectoryCount >> 4;
	_fat.clusterStart = _fat.rootStart + _fat.rootCount;

	//	Calculate root count for FAT32 
	if (_fat.rootCluster)
	{
		Extent extents[1];
		extents[0].start = 2;
		extents[0].count = 1;
		LoadExtents(extents,1);
		_fat.rootCount = extents[0].count * _fat.sectorsPerCluster; // # of sectors in first cluster of root
	}
	return _fat.rootCluster ? FAT_32 : FAT_16;
}

//  Cache the last file opened...
DirectoryEntry _last_directory_entry;

#ifdef SIMULATOR
int SimFAT_Directory(DirectoryProc directoryProc, u8* buffer, void* ref);
int FAT_Directory(DirectoryProc directoryProc, u8* buffer, void* ref)
{
	return SimFAT_Directory(directoryProc,buffer,ref);
}

#else
//	Traverse Directory
//	TODO: This won't work properly on fragmented root directories; unlikely in our application
int FAT_Directory(DirectoryProc directoryProc, u8* buffer, void* ref)
{
	// cache hit
	if (_last_directory_entry.fatname[0] && directoryProc(&_last_directory_entry, 0, ref))
		return 0x7FFF;

	_fat.buffer = buffer;
	for (u16 i = 0; i < _fat.rootCount; i++)
	{
		if (_fat.readProc(_fat.buffer, _fat.rootStart + i))
			return -1;	// Dir read failed

		u8 d = 16;
		DirectoryEntry* entry = (DirectoryEntry*)_fat.buffer;
		while (d--)
		{
			if (entry->fatname[0] == 0)
				return -1;
			
			if ((u8)entry->fatname[0] == 0xE5 || (entry->attributes & 0x18))
			{
				//	deleted, dir, volume label etc
			} else {
				//	file
				if (directoryProc(entry,i,ref))
					return i;
			}
			entry++;
		}
	}
	return -1;
}
#endif

typedef struct
{
	char name[11];
	DirectoryEntry* entry;
} MatchName;

static bool MatchProc(DirectoryEntry* entry, int i, void* ref)
{
	MatchName* matchName = (MatchName*)ref;
	if (memcmp(entry->fatname, matchName->name, 11))
		return false;
	*matchName->entry = *entry;
	return true;
}

u32 NextCluster(u32* s, u32 currentCluster)
{
	bool fat32 = _fat.rootCluster != 0;
	u32 fatSector = currentCluster >> (fat32 ? 7 : 8);
	fatSector += _fat.fatStart;
	if (fatSector != *s)
	{
		*s = fatSector;
		MMC_ReadSector(_fat.buffer,fatSector);	// Read sector containing next cluster
	}
	if (fat32)
		return ((u32*)_fat.buffer)[currentCluster & 0x7F];
	return ((u16*)_fat.buffer)[currentCluster & 0xFF];
}

//	File is bigger than a single cluster
//	Load first 3 extents
//	Bad news: Opening a very large file takes a long time
//	Good news: unless file is badly fragmented, subsequent acceses will be much faster
void LoadExtents(Extent* extents, int maxExtents)
{
	u8 i = 0;
	u32 state = 0;
	u32 current = extents[0].start;
	for (;;)
	{
		u32 next = NextCluster(&state,current);
		if (next != current + 1)
		{
			if (i == maxExtents-1)
				break;
			if (next == (_fat.rootCluster ? 0x0FFFFFFF : 0xFFFF))
				break;
			extents[++i].start = next;
			extents[i].count = 0;
		}
		extents[i].count++;
		current = next;
	}
	while (i < maxExtents-1)
		extents[++i].start = 0;
}

#define UPPER(_x) ((_x >= 'a' && _x <= 'z') ? (_x + 'A' - 'a') : _x)

void ToFAT(char fatname[11], const char* path)
{
	//	Convert name to 8.3 internal format
	u8 i = 11;
	while (i--)
		fatname[i] = ' ';
	i = 0;
	bool ext = false;
	while (i < 11)
	{
		char c = *path++;
		if (c == 0)
			break;
		c = UPPER(c);
		if (c == '.')
		{
			i = 8;
			ext = true;
		}
		else
		{
			if (i < 8 || ext)
				fatname[i++] = c;
		}
	}
}

//	Find a file entry
bool FAT_Find(const char* path, u8* buffer, DirectoryEntry* entry)
{
	MatchName matchName;
	ToFAT(matchName.name,path);
	matchName.entry = entry;
	int index = FAT_Directory(MatchProc,buffer,&matchName);
	return index != -1;
}

//	Load extents when file is open. TODO: short check for very large files
bool FAT_Open(const char* path, ExtentInfo* extent, u8* buffer)
{
	_fat.buffer = buffer;
	DirectoryEntry entry;

	if (!FAT_Find(path,buffer,&entry))
		return false;

	extent->fileLength = entry.length;
	u32 cluster = entry.clusterH;
	extent->extents[0].start = (cluster << 16) | entry.clusterL;
	extent->extents[0].count = 1;
	extent->extents[1].start = 0;
	if (extent->fileLength > (_fat.sectorsPerCluster<<9))
		LoadExtents(extent->extents,3);
	return true;
}

//	TODO: read beyond 3 extent!
//	TODO: Store extents as absolute sectors rather than clusters, avoid muls

bool FAT_FindSector(u32* sector, ExtentInfo* extent)
{
	u8 i = 0;
	while (i < 3)
	{
		u32 s = extent->extents[i].count*_fat.sectorsPerCluster;
		if (*sector < s)
		{
			u32 cluster = extent->extents[i].start;
			*sector += _fat.clusterStart + (cluster - 2)*_fat.sectorsPerCluster;
			return true;
		}
		*sector -= s;
		i++;
	}
	// TODO: Deal with nasty fragmentation
	return false;
}

bool FAT_Read(u8* buffer, u32 sector, ExtentInfo* extent)
{
	if (!FAT_FindSector(&sector,extent))
		return false;
	return MMC_ReadSector(buffer,sector) == 0;
}

bool FAT_Write(u8* buffer, u32 sector, ExtentInfo* extent)
{
	if (!FAT_FindSector(&sector,extent))
		return false;
	return MMC_WriteSector(buffer,sector) == 0;
}

//	helper to convert fatname in DirectoryEntry to string
const char* FAT_Name(char* name, DirectoryEntry* entry)
{
	u8 i = 0;
	char* fatname = entry->fatname;
	char* n = name;
	char c;
	while (i < 8)
	{
		c = fatname[i];
		if (c == ' ')
			break;
		*n++ = c;
		i++;
	}
	if (fatname[8] != ' ')
	{
		*n++ = '.';
		i = 8;
		while (i < 11)
		{
			c = fatname[i++];
			if (c == ' ')
				break;
			*n++= c;
		}
	}
	*n = 0;
	return name;
}

// Read

#include "File.h"
static bool TestProc(DirectoryEntry* entry, int i, void* ref)
{
	u8* oldBuffer = _fat.buffer;
	char name[16];
	File file;
	if (file.Open(FAT_Name(name,entry)))
	{
		u8 head[512];
		file.Read(head,sizeof(head));
	}
	_fat.buffer = oldBuffer;
	return false;
}

void FAT_Test()
{
	u8 buffer[512];
	FAT_Directory(TestProc,buffer,0);
}
