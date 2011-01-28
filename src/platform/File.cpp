
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
#ifdef USE_WIN32_FS
	CloseHandle(_h);
#endif
}

void File::Init()
{
    _mark = 512;
    _extent.fileLength = 0;
    _sector = -1;
#ifdef USE_WIN32_FS
	_h = INVALID_HANDLE_VALUE;
#endif
}

byte File::Open(const char* path)
{
#ifdef USE_WIN32_FS
    char s[1024];
    sprintf(s,"microSD\\%s",path);
    if (_h != INVALID_HANDLE_VALUE)
        ::CloseHandle(_h);
    _h = ::CreateFileA(s,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL );
    Load(0);
    _extent.fileLength = GetFileSize( _h, NULL );
    return _h == INVALID_HANDLE_VALUE ? 0 : 1;
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

void File::Load(long sector)
{
    _mark = 0;
    if (_sector == sector)
        return;
    _sector = sector;

#ifdef USE_WIN32_FS
	DWORD r;
	::SetFilePointer(_h,_sector*512,0,0);
	::ReadFile(_h,_buffer,512,&r,NULL);
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