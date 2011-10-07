
/* Copyright (c) 2009, Peter Barrett  
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

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <stdio.h>
#include <string.h>

#define MAX_APP_BUFFER 768

// Draconian measures to fit large apps (like frotz etc)
#ifdef MIN_FLASH_SIZE
#define DISABLE_USB
#define DISABLE_PROFILER
#endif

#ifndef byte
typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned long ulong;
#endif

#ifndef uchar
typedef unsigned char uchar;
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

u8 MMC_Init();
u8 MMC_ReadSector(u8* buffer, u32 sector);
u8 MMC_WriteSector(u8* buffer, u32 sector);

void quicksort(int arr[], int left, int right);
#define bound(_x,_min,_max) ((_x) < (_min) ? (_min) : (((_x) > (_max)) ? (_max) : (_x)))

#ifdef _WIN32
#define SIMULATOR
#endif

#ifdef _QT
#define SIMULATOR
#endif

#ifndef SIMULATOR
#include <inttypes.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#ifndef abs
#define abs(_x) ((_x) < 0 ? -(_x) : (_x))
#endif
#define CONST_PCHAR const prog_char*
#define ROMSTRING(_s) extern const char _s[] PROGMEM


class PStr
{
	char _str[32];
public:
	PStr(const char* pstr)
	{
		strncpy_P(_str,pstr,sizeof(_str));
	}
	operator const char*()
	{
		return _str;
	}
};

#else

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <stdlib.h>
#include <stdarg.h>

#define printf_P printf
#define sprintf_P sprintf

#define ROMSTRING(_s)
#define PROGMEM 
#define CONST_PCHAR const char *
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
//typedef unsigned long uint32_t;
#define pgm_read_byte(_x) (*(_x))
#define pgm_read_word(_x) (*((short*)(_x)))
#define memcpy_P memcpy
#define strcmp_P strcmp
#define strncpy_P strncpy
#define strcpy_P strcpy
#define PStr

void _delay_ms(int ms);

#define USE_STDIO
#endif


#ifndef max
#define max(_a,_b) (((_a) > (_b)) ? (_a) : (_b))
#define min(_a,_b) (((_a) < (_b)) ? (_a) : (_b))
#endif

u32 RandomBits(u8 bits);
void delay(ushort ms);
void assertFailed(const char* str, int len);

#ifdef MIN_FLASH_SIZE
#define ASSERT(_x)
#else
#define ASSERT(_x) if (!(_x)) assertFailed(NULL,__LINE__);
#endif

class TouchData
{
public:
    int x;
    int y;
	int pressure;
};


//=============================================================================
//=============================================================================
//	Events
//	Apps support a simple event api
//	e is an event
//	appState is memory for app that is preserved between calls to AppProc
//	return -1 to kill the app

class Event
{
public:
	enum EType
    {
        None,
		OpenApp,
		CloseApp,

        TouchDown,
        TouchMove,
        TouchUp,

		MicroSDInserted,
		MicroSDRemoved,

		USBAttach,
		USBDetach,
		USBPacket
    };
	EType Type;
	union {
		void* Data;
		TouchData* Touch;
	};
};

typedef int (*AppProc)(Event* e, void* appState);

//	Application installer macro
//	_n is the human readable name of the application
//	_s is an object with a OnEvent method

#define _INSTALL_APP(_n,_s) \
	ROMSTRING(S_##_n); \
	const char S_##_n[] = #_n; \
	int proc_##_n(Event* e, void* s) {return ((_s*)s)->OnEvent(e);} \
	RegisterApp app_##_n(S_##_n,(AppProc)proc_##_n,sizeof(_s))

// Single Application replaces shell
#ifdef NO_SHELL
#define INSTALL_APP(_n,_s) _INSTALL_APP(shell,_s)
#else
#define INSTALL_APP(_n,_s) _INSTALL_APP(_n,_s)
#endif

//	Auto register the app with the Shell, will be called before Shell_Init
class RegisterApp
{
public:
	RegisterApp(const char* name, AppProc proc, int stateSize);
};

void Shell_Init();
void Shell_Loop();
bool Shell_GetAppName(int index, char* name, int len);
void Shell_LaunchApp(const char* params);
void Shell_LaunchFile(const char* filename);
const char* Shell_AppName();
int Shell_AppCount();


#include "Graphics.h"
#include "File.h"
#include "Hardware.h"

#endif // __PLATFORM_H__
