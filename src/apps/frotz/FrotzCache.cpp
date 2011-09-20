
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

typedef unsigned char zchar;
typedef unsigned char zbyte;
typedef unsigned short zword;

extern "C"
{
	extern zbyte h_version;
	extern zword h_dynamic_size;
	extern zword h_checksum;

	extern long pcp;
	extern zword sp;
	extern zword fp;

	void	ret (zword);
	void 	store (zword);
	void 	branch (bool);
};

//648 in this config
#define CACHEBITS	4	// 4 (119,8) 3(156,7) 5(106,11) 6(109,14) 7(117,17)
#define CACHEMASK	((1 << CACHEBITS)-1)
#define CACHELINE	(1<<CACHEBITS)
#define CACHEMEM	592	// 592
#define CACHESLOTS	(CACHEMEM/CACHELINE)


class PCache
{
public:
	u8 _head;
	u8 _next[CACHESLOTS];
	u8 _flags[CACHESLOTS];
	u16 _addrs[CACHESLOTS];
	u8 _data[CACHEMEM];
	File* _file;

	void Init(File* f)
	{
		_file = f;
		u8 n = CACHESLOTS-1;
		_next[n] = 0xFF;
		while (n--)
			_next[n] = n+1;
		memset(_flags,0,sizeof(_flags));
		memset(_addrs,0xFF,sizeof(_addrs));
		_head = 0;
	}

	u8* PageData(u8 n)
	{
		return _data + n*CACHELINE;
	}

	u16 ToSector(u16 paddr)
	{
		return paddr >> (9 - CACHEBITS);
	}

	u8* Seek(u16 paddr)
	{
		_file->SetPos(((long)paddr) << CACHEBITS);
		int n;
		return (u8*)_file->GetBuffer(&n);
	}

	// Need to write back this sector
	void FlushSector(u16 addr)
	{
		u16 s = ToSector(addr);
		int n = CACHESLOTS;
		while (n--)
		{
			if (_flags[n])	// Flush all
			{
				if (s == ToSector(_addrs[n]))
				{
					Seek(_addrs[n]);
					u8* d = PageData(n);
					_file->Write(d,CACHELINE);
					_flags[n] = 0;
				}
			}
		}
	}

	u8* Miss(u16 paddr)
	{
		u8 n = _head;
		if (_flags[n])
		{
			u16 spm = sp >> (CACHEBITS-1);
			u16 a = _addrs[n];
			if (a >= spm)	// Don't flush old stack junk
				FlushSector(a);
		}

		u8* dst = PageData(n);
		memcpy(dst,Seek(paddr),CACHELINE);	// LOAD
		_addrs[n] = paddr;
		_flags[n] = 1;
		return dst;
	}

	void BringToFront(u8 slot, u8 prev)
	{
		if (slot == _head)
			return;
		_next[prev] = _next[slot];
		_next[slot] = _head;
		_head = slot;
	}

	u8* Get(u16 addr, bool w)
	{
		u8 prev = 0xFF;
		u8 slot = _head;
		for (;;)
		{
			if (_addrs[slot] == addr)	// LRU
			{
				_flags[slot] |= w;
				BringToFront(slot,prev);
				return PageData(slot);
			}
			if (_next[slot] == 0xFF)
				break;
			prev = slot;
			slot = _next[slot];
		}

		if (!w)
			return Seek(addr);

		BringToFront(slot,prev);	// keep
		return Miss(addr);
	}
	
	u8* GetA(long a, bool w)
	{
		u16 h = (a >> CACHEBITS);
		u16 l = a;
		return Get(h,w) + (l & CACHEMASK);
	}

	zword* GetStack(zword s, bool w)
	{
		return (zword*)GetA(s+s,w);
	}

	u8* GetAddr(long a, bool w)
	{
		return GetA(a + 2048 + 32*1024L, w);
	}
};

PCache* _pc = 0;
File _pageFile;



extern "C"
{

zword* GetStack(zword x)
{
	//printf("s");
	return _pc->GetStack(x,false);
}

zword* GetStackW(zword x)
{
	//printf("S");
	return _pc->GetStack(x,true);
}

void SetZByte(long a, zchar v)
{
	//printf("Z");
	*(_pc->GetAddr(a,true)) = v;
}

zchar GetZByte(long a)
{
	//printf("z");
	return *(_pc->GetAddr(a,false));
}

void SetZWord(long a, zword v)
{
	SetZByte(a,v >> 8);
	SetZByte(a+1,v);
}

zword GetZWord(long a)
{
	int h = GetZByte(a);
	u8 l = GetZByte(a+1);
	return (h << 8) | l;
}

u8* Scrollback(zword p, bool write)
{
	return _pc->GetA(p + 2048,write);
}

long pcp;

//	These are read only and long (frequent)

u8 ccache[128];
u8 cmark;
u32 cpage = -1;
#define cmask (~127L)

void SetCPage()
{
	u32 page = pcp & cmask;
	if (page != cpage)
	{
		cpage = page;
		long p = (page + 2048 + 32*1024L);
		_pc->Seek(p >> CACHEBITS);
		_pc->_file->Read(ccache,128);
	}
	cmark = pcp & 0x7F;
}

void SetPC(long a)
{
	pcp = a;
	SetCPage();
}

long GetPC()
{
	return pcp;
}

zbyte GetCByte()
{
	if (cmark == 128)
		SetCPage();
	pcp++;
	return ccache[cmark++];
}

zword GetCWord(long a)
{
	int h = GetCByte();
	u8 l = GetCByte();
	return (h << 8) | l;
}

}

zword ReadWord(long a)
{
	_pageFile.SetPos(a);
	zword w;
	_pageFile.Read(&w,2);
	return w;
}

extern const char S_gamez5[] PROGMEM;
extern const char S_ppge[] PROGMEM;

const char S_gamez5[] = "game.z5";
const char S_ppge[] = "p.pge";

void OpenZFile(u8* cacheMem, int space)
{
	//	use cachemem
	File* f = (File*)cacheMem;
	f->Init();

	f->Open(PStr(S_gamez5));

	_pageFile.Init();
	_pageFile.Open(PStr(S_ppge));
	_pageFile.SetPos(2048 + 32*1024L);

	//	Copy to page file
	int n = (f->GetFileLength() + 511) >> 9;
	while (n--)
	{
		int count;
		const u8* b = f->GetBuffer(&count);
		_pageFile.Write(b,512);
		f->Skip(count);
	}

	_pc = (PCache*)cacheMem;
	ASSERT(sizeof(PCache) < space);
	_pc->Init(&_pageFile);
}


//	Buffered to avoid thrashing?
class SaveGame
{
	long _mark;
public:
	SaveGame() : _mark(256*1024L)	// Start of save game
	{
	}
	void Write(u16 w)
	{
		Write8(w);
		Write8(w >> 8);
	}
	void Write8(u8 b)
	{
		SetZByte(_mark++,b);
	}
	u16 Read()
	{
		u16 a = Read8();
		return a | (((u16)Read8()) << 8);
	}
	u8 Read8()
	{
		return GetZByte(_mark++);
	}
};


zword* GetStack(zword x);
zchar  GetZByte(long a);

void ShowStatus(const char* s);
extern const char S_Saving[] PROGMEM;
const char S_Saving[] = "Saving...";
extern const char S_Restoring[] PROGMEM;
const char S_Restoring[] = "Restoring...";

extern "C"
void z_save()
{
	ShowStatus(PStr(S_Saving));

	zword success = 1;
	SaveGame game;

	game.Write(h_checksum);
	game.Write8(0);		// name string

	game.Write(pcp);	// PC
	game.Write(pcp >> 16);
	game.Write(sp);		// stack
	game.Write(fp);		// frame pointer

	//	Write stack
	for (u16 i = sp; i < 1024; i++)
		game.Write(*GetStack(i));

	//	Size of dynamic area
	game.Write(h_dynamic_size);
	for (u16 i = 0; i < h_dynamic_size; i++)
		game.Write8(GetZByte(i));

    if (h_version <= 3)
		branch (success);
    else
		store (success);
}


extern "C"
void z_restore (void)
{
    u8 success = 1;
	ShowStatus(PStr(S_Restoring));

	SaveGame game;
	if (game.Read() == h_checksum)
	{
		u8 n;
		while ((n = game.Read8()) != 0)
			;	// Read name

		long pc = game.Read();
		pc |= ((long)game.Read()) << 16;
		sp = game.Read();
		fp = game.Read();

		//	Read stack
		for (u16 i = sp; i < 1024; i++)
			*GetStackW(i) = game.Read();

		//	Size of dynamic area
		/*int ds = */game.Read();	// must == h_dynamic_size
		for (u16 i = 0; i < h_dynamic_size; i++)
			SetZByte(i,game.Read8());

		SetPC(pc);
	} else
		success = 0;

    if (h_version <= 3)
		branch (success);
    else
		store (success);
}/* z_restore */