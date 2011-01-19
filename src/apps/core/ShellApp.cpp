
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

int USBGetChar();

ROMSTRING(S_viewname);
const char S_viewname[] = "view";

//	Basic shell shows built in apps and loadable files on microsd if present

#define CELL_WIDTH 80
#define CELL_HEIGHT 80
#define CELL_COLS 3
#define MAX_APPS 16
#define MAX_FILES 16
#define MAX_ITEMS (MAX_APPS + MAX_FILES)

#define MAX_SHELL_CHARS 31

bool lsProc(DirectoryEntry* d, int index, void* ref)
{
	char buffer[16];
	printf(FAT_Name(buffer,d));
	putchar(10);
	return false;
}

uint16_t ReadLcdReg(uint16_t addr);
ROMSTRING(LCDPrint);
const char LCDPrint[] = "%02X: %04X\r\n";

class ShellState
{
	int _appCount;
	int _fileCount;
	int _tracking;
	int _last;

	u8 _appMap[MAX_APPS];

	u8 _phase[MAX_ITEMS];
	char _files[16*MAX_FILES];
	char _shellLine[MAX_SHELL_CHARS+1];
	u8 _shellCount;

public:
	int ToIndex(int x, int y)
	{
		x /= CELL_WIDTH;
		y /= CELL_HEIGHT;
		x += y*CELL_COLS;
		return x;
	}

	const char* GetFileName(u8 index)
	{
		return _files + ((index-_appCount)<<4);
	}

	void GetName(u8 index, char* name, int len)
	{
		if (index < _appCount)
		{
			Shell_GetAppName(_appMap[index],name,len);
			return;
		}

		//	Indexed file name?
		strcpy(name,GetFileName(index));
	}

	void Draw(u8 index, int hilite, int radius = CELL_WIDTH/4)
	{
		char name[16];
		GetName(index,name,sizeof(name));

		int textColor = 0;
		int c = RED;
		if (hilite)
			textColor = c;
		else
		{
			c = name[0];
			u8 i = 0;
			while (name[i])
				c ^= c*name[i++];	// color hash
		}

		int x = index % CELL_COLS;
		int y = index / CELL_COLS;
		x = x*CELL_WIDTH + CELL_WIDTH/2;
		y = y*CELL_HEIGHT + CELL_HEIGHT/2;
		Graphics.Circle(x-1,y-1,radius,c,true);

		if (radius != CELL_WIDTH/4)
			return;

		int len = strlen(name);
		int w = Graphics.MeasureString(name,len);
		Graphics.DrawString(name,len,x-w/2,y+CELL_HEIGHT/2 - 14,GRAY(0xc0));
		Graphics.DrawString(name,len,x-w/2-1,y+CELL_HEIGHT/2 - 14-1,textColor);
	}

	void Splash(int index)
	{
		int r = CELL_WIDTH/4;
		while (r < 256)
		{
			Draw(index,false,r);
			r = r + r;
		}
		Graphics.Clear(0xFFFF);
	}

	void Launch(int index)
	{
		Splash(index);
		if (index < _appCount)
		{
			char name[32];
			Shell_GetAppName(_appMap[index],name,sizeof(name));
			Shell_LaunchApp(name);
		} else {
			Shell_LaunchFile(GetFileName(index));
		}
	}

	void SerialCommand(const char* cmd)
	{
		if (cmd[0] == 'l')
		{
			//	Dump LCD registers
			if (cmd[1] == 'c' && cmd[2] == 'd')
			{
				for (u8 i = 0; i < 0x99; i++)
				{
					int reg = ReadLcdReg(i);
					if (reg)
						printf_P(LCDPrint,i,reg);
				}
				return;
			}

			//	Dump fat (once mounted todo)
			if (cmd[1] == 's')
			{
				u8 buffer[512];
				FAT_Directory(lsProc,buffer,0);
				return;
			}
		}
		else if ((cmd[0] == 'p') && (cmd[2] == 0))	// Turn on/off profiling
		{
			if (cmd[1] == '0' || cmd[1] == '1')
			{
				Hardware.Profile(cmd[1] == '1');
				return;
			}
		}


		//	Is it an app?
		u8 i = Shell_AppCount();
		while (i--)
		{
			char name[32];
			Shell_GetAppName(i,name,sizeof(name));
			if (strncmp(name,cmd,strlen(name)) == 0)
			{
				Graphics.Clear(0xFFFF);
				Shell_LaunchApp(name);
				return;
			}
		}

		// unrecognized command
		putchar('?');
	}

	// read commands from serial input
	void SerialLoop()
	{
		int n = USBGetChar();
		if (n < 0)
			return;
		u8 c = n;
		switch (c)
		{
			case 13:	// enter
				if (_shellCount)
				{
					_shellLine[_shellCount] = 0;
					_shellCount = 0;
					putchar(10);
					SerialCommand(_shellLine);
					putchar(10);
					putchar('>');
				}
				break;
			case 127:	// bkspace
				if (_shellCount)
					_shellCount--;
				break;
			default:
				if (_shellCount < MAX_SHELL_CHARS)
					_shellLine[_shellCount++] = c;
				putchar(c);
				break;
		}
	}

	// File iteration callback
	static bool OnFile(DirectoryEntry* entry, int index, void* ref)
	{
		ShellState* s = (ShellState*)ref;
		if (s->_fileCount == MAX_FILES)
			return true;
		FAT_Name(s->_files + (s->_fileCount<<4),entry);
		s->_fileCount++;
		return false;
	}

	void Init()
	{
		//	Count built in apps
		_appCount = 0;
		for (u8 i = 1; i < Shell_AppCount(); i++)
		{
			char name[32];
			Shell_GetAppName(i,name,sizeof(name));
			if (strcmp_P(name,S_viewname) == 0)	/// don't display view app
				continue;
			_appMap[_appCount++] = i;
			if (_appCount == MAX_APPS)
				break;
		}

		//	Count Files
		{
			u8 buffer[512];
			_fileCount = 0;
			FAT_Directory(OnFile,buffer,this);
		}

		Graphics.Clear(0xFFFF);
		{
			u8 i = _appCount + _fileCount;
			while (i--)
				_phase[i] = RandomBits(6) & 63;	//
		}
		_tracking = -1;
		_shellCount = 0;
	}

	int OnEvent(Event* e)
	{
		switch (e->Type)
		{
			case Event::OpenApp:
				Init();
				break;

			case Event::None:
				{
					u8 i =  _appCount + _fileCount;
					while (i--)
					{
						if (_phase[i] != 0x3F)
						{
							int r = ++_phase[i];
							r -= 63-CELL_WIDTH/4;
							if (r > 0)
								Draw(i,false,r);
						}
					}
					SerialLoop();
				}
				break;

			case Event::TouchDown:
				{
					TouchData* t = (TouchData*)e->Data;
					int i = ToIndex(t->x,t->y);
					if (i < 0 || i >= (_fileCount+_appCount))
						break;

					Draw(i,true);
					_tracking = i;
					_last = i;
				}
				break;

			case Event::TouchMove:
				{
					TouchData* t = (TouchData*)e->Data;
					int i = ToIndex(t->x,t->y);
					if (_tracking != -1 && i != _last)
					{
						Draw(_tracking,i == _tracking);
						_last = i;
					}
				}
				break;

			case Event::TouchUp:
				if (_tracking != -1)
				{
					if (_tracking == _last)
					{
						Launch(_tracking);
						return -1;
					}
					_tracking = -1;
				}
				break;

			default:;
		}
		return 0;
	}
};

//	Register the application, start getting events on OnEvent method
INSTALL_APP(shell,ShellState);