

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

#include "Platform.h"

//=============================================================================
//=============================================================================

//	Default file viewer
extern const char DEFAULT_FILE_VIEWER[] PROGMEM;
const char DEFAULT_FILE_VIEWER[] = "view ";

#define MAX_APPS 15	// Maximum number of built in apps
//#define PRINT_FPS	// Prints frames per second

typedef struct		// Apps register themselves from static constructors
{
	CONST_PCHAR name;
	AppProc	proc;
	int	stateLen;
} AppDefinition;

int _appCount = 0;
AppDefinition _appList[MAX_APPS];

class Shell
{
	u8	_buffer[MAX_APP_BUFFER];	// Applications run in this buffer

	TouchData _lastTouch;
	u8	_appIndex;
	int	_appResult;
	AppProc	_proc;
	char _launch[32];

#ifdef PRINT_FPS
	int _count;
	u32 _ticks;
#endif

public:
	void Init()
	{
		Graphics.Init();
		Graphics.Clear(RED);

		//	Will fail if no card inserted
		//	TODO: Polling for insterion?
		MMC_Init();
		FAT_Init(_buffer,MMC_ReadSector);

		_appResult = 0;
		_proc = 0;
		_appIndex = 0;
		_launch[0] = 0;
	}

	void Launch(const char* app)
	{
		strncpy(_launch,app,sizeof(_launch));
	}

	void LaunchFile(const char* file)
	{
		strcpy_P(_launch,DEFAULT_FILE_VIEWER);
		strcpy(_launch + strlen(_launch),file);
	}

	AppProc FindApp(const char* name)
	{
		_appIndex = _appCount;
		while (_appIndex--)
		{
			if (strcmp_P(name,_appList[_appIndex].name) == 0)	// TODO
				return _appList[_appIndex].proc;
		}
		_appIndex = 0;
		return _appList[0].proc;	// not found, run shell
	}

	const char* AppName()
	{
		return _appList[_appIndex].name;
	}

	void Loop()
	{
		//	Launch an app with params based on name
		if (_proc == 0 || _launch[0])
		{
			if (!_launch[0])
				strncpy_P(_launch,_appList[0].name,sizeof(_launch));	// back to shell

			char* param = _launch;
			while (*param && *param != ' ')
				*param++;
			if (*param)
				*param++ = 0;	// Split app name and params

			_proc = FindApp(_launch);
			Event e;
			e.Type = Event::OpenApp;
			e.Data = param;			// ptr to params
			_proc(&e,_buffer);		// Open app
			_launch[0] = 0;
#ifdef PRINT_FPS
			_ticks = millis() + 1000;
			_count = 0;
#endif
		}

		Event e;
        e.Type = Event::None;
		TouchData t;

        if (Hardware.GetTouch(&t))
		{
			if (_lastTouch.pressure)
			{
				if ((t.x != _lastTouch.x) || (t.y != _lastTouch.y) || (t.pressure != _lastTouch.pressure))
					e.Type = Event::TouchMove;
			} else
				e.Type = Event::TouchDown;
		}
		else 
		{
			if (_lastTouch.pressure)
				e.Type = Event::TouchUp;
		}
		e.Data = &t;
		_lastTouch = t;

		//	Run app
		_appResult = _proc(&e,_buffer);
		if (_appResult != 0)
		{
			// Save state
			//SaveStateEvent s;
			//_proc(&s,_buffer,sizeof(_buffer));
			_proc = 0;			// App is terminating
		}

#ifdef PRINT_FPS
		_count++;
		u32 ticks = millis();
		if (ticks > _ticks)
		{
			printf("%d\n",_count);
			_ticks += 1000;
			_count = 0;
		}
#endif
	}
};

Shell gShell;

//	Default app is shell
int proc_shell(Event* e, void* state);

//	Auto register the app with the Shell, will be called before Shell_Init
RegisterApp::RegisterApp(const char* name, AppProc proc, int stateLen)
{
	if (_appCount == MAX_APPS || (stateLen > MAX_APP_BUFFER))
		return;

	AppDefinition& app = _appList[_appCount++];
	app.name = name;
	app.proc = proc;
	app.stateLen = stateLen;

	//	Swap shell app to first in list
	if (proc_shell == proc)
	{
		AppDefinition tmp = _appList[0];
		_appList[0] = app;
		app = tmp;
	}
};

void Shell_Init()
{
	gShell.Init();
}

void Shell_Loop()
{
	gShell.Loop();
}

int Shell_AppCount()
{
	return _appCount;
}

bool Shell_GetAppName(int index, char* name, int len)
{
	if (index < 0 || index >= _appCount)
		return false;
	strncpy_P(name,_appList[index].name,len);
	return true;
}

void Shell_LaunchApp(const char* params)
{
	gShell.Launch(params);
}

void Shell_LaunchFile(const char* filename)
{
	gShell.LaunchFile(filename);
}

//	Name of current app
const char* Shell_AppName()
{
	return gShell.AppName();
}
