
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

File::File()
{
	Init();
}

File::~File()
{
	Close();
}

void File::Init()
{
    _mark = 512;
    _extent.fileLength = 0;
    _sector = -1;
	_dirty = 0;

#ifdef SIMULATOR
	_f = 0;
#endif
}

byte File::Close()
{
	if (_dirty)
		Flush();
	_sector = -1;
#ifdef SIMULATOR
	if (_f)
        fclose(_f);
	_f = 0;
#endif
	return 0;
}

byte File::Open(const char* path)
{
#ifdef SIMULATOR
    char s[1024];
    sprintf(s,"microSD/%s",path);
    if (_f)
        fclose(_f);
    _f = fopen(s,"r+b");
    if (_f)
    {
        fseek(_f,0,SEEK_END);
        _extent.fileLength = ftell(_f);
        fseek(_f,0,SEEK_SET);
        Load(0);
    }
    return _f ? 1 : 0;
#else

    if (FAT_Open(path,&_extent,_buffer))
    {
        Load(0);    // load 0?
        return 1;
    }
    #if 0
        char s[32];
        sprintf(s,"Opened %s %ld:%ld\n",path,_origin,_fileLength);
        print(s);
        Console(s);
    #endif
    return 0;
#endif
}

ulong File::GetFileLength()
{
    return _extent.fileLength;
}

byte File::ReadByte()
{
    if (_mark == 512)
        Load(_sector + 1);
    return _buffer[_mark++];
}
      
int File::Read(void* d, int len)
{
    byte* dst = (byte*)d;
    for (int i = 0; i < len; i++)
        dst[i] = ReadByte();
    return len;
}

void File::WriteByte(byte b)
{
    if (_mark == 512)
        Load(_sector + 1);
    if (b != _buffer[_mark])
    {
            _dirty = 1;
            _buffer[_mark] = b;
    }
    _mark++;
}

int File::Write(const void* d, int len)
{
    const byte* src = (byte*)d;
    for (int i = 0; i < len; i++)
		WriteByte(src[i]);
    return len;
}

const byte* File::GetBuffer(int* count)
{
    if (_mark == 512)
        Load(_sector + 1);
    *count = sizeof(_buffer) - _mark;
    return _buffer + _mark;
}

void File::Skip(int count)
{
    _mark += count; // 
    if (_mark > 0x200)
    {
        _mark -= count;
        SetPos(GetPos() + count);
    }
    ASSERT(_mark >= 0 && _mark <= 0x200);
}

void File::Flush()
{
#ifdef SIMULATOR
        //printf("w%d ",_sector);
        fseek(_f,_sector*512,SEEK_SET);
        int e = fwrite(_buffer,512,1,_f);
#else
	FAT_Write(_buffer,_sector,&_extent);
#endif
	_dirty = 0;
}

void File::Load(long sector)
{
    _mark = 0;
    if (_sector == sector)
        return;
	if (_dirty)
		Flush();
    _sector = sector;

#ifdef SIMULATOR
        //printf("r%d ",sector);
        fseek(_f,_sector*512,SEEK_SET);
        fread(_buffer,512,1,_f);
#else
	FAT_Read(_buffer,sector,&_extent);
#endif
}

long  File::GetPos()
{
    return (_sector << 9) + _mark;
}

void  File::SetPos(long pos)
{
    Load(pos >> 9);
    _mark = pos & 0x1FF;
}

#ifdef SIMULATOR
#include <dirent.h>
#include <ctype.h>

static
void ToFATName(char* dst, const char* src)
{
    memset(dst,' ',11);
    char c;
    for (int i = 0; i < 8; i++)
    {
            c = *src++;
            if (c == '.')
                    break;
            if (!c)
                    return;
            dst[i] = toupper(c);
    }
    while (c != '.')
    {
            c = *src++;
            if (!c)
                    return;
    }
    for (int i = 8; i < 11; i++)
    {
            c = *src++;
            if (!c)
                    return;
            dst[i] = toupper(c);
    }
}


int SimFAT_Directory(DirectoryProc directoryProc, u8* buffer, void* ref)
{
    DIR* dir = opendir("microSD");
    if (dir == 0)
        return -1;

    DirectoryEntry entry;
    struct dirent *dp;
    int index = 0;
    int result = -1;
    while ((dp = readdir (dir)) != NULL)
    {
        if (dp->d_name[0] == '.')
            continue;
        memset(&entry,0,sizeof(entry));
        ToFATName(entry.fatname,dp->d_name);
        if (directoryProc(&entry,index,ref))
        {
            result = index;
            break;
        }
        index++;
    }
    closedir(dir);
    return result;
}
#endif
