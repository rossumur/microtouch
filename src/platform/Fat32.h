
/* Copyright (c) 2009,2010, Peter Barrett  
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

#ifndef __FAT32_H__
#define __FAT32_H__

typedef struct
{
	u32 start;
	u32 count;
} Extent;

typedef struct
{
	u32 fileLength;
	Extent extents[3];	// First 3 extents
} ExtentInfo;

typedef struct
{
   char fatname[11];
   u8	attributes;
   u8	reserved[8];
   u16	clusterH;
   u16	time;
   u16	date;
   u16	clusterL;
   u32	length;
} DirectoryEntry;

typedef u8 (*ReadProc)(u8* buffer, u32 sector);
typedef bool (*DirectoryProc)(DirectoryEntry* d, int index, void* ref);

#define FAT_NONE 0
#define FAT_16 16
#define FAT_32 32

u8		FAT_Init(u8* buffer, ReadProc readProc);

bool	FAT_Open(const char* path, ExtentInfo* extent, u8* buffer);
bool	FAT_Read(u8* buffer, u32 sector, ExtentInfo* extent);
bool	FAT_Write(u8* buffer, u32 sector, ExtentInfo* extent);
int		FAT_Directory(DirectoryProc directoryProc, u8* buffer, void* ref);
const char* FAT_Name(char* name, DirectoryEntry* entry);

#endif
