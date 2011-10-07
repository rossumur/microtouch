# Welcome to Microtouch

Microtouch is a little device powered by the delightful Atmega32u4.
It features a 320x240 pixel touchscreen, an accelerometer, full speed
usb, a microsd card reader and a litium ion battery.

It has an application framework of sorts and it is possible to run a
variety of applications with varying degrees of utility. Possibilites
are bounded only by your imagination (and the 8 bit cpu + 2.5k of RAM).

## Getting Started

* Buy the hardware from the lovely folks at Adafruit (or diy if brave)
* Install WinAVR ([a more detailed description here](http://winavr.sourceforge.net/install_config_WinAVR.pdf))
* For linux users install the avr tool chain with

		sudo apt-get install gcc-avr binutils-avr avr-libc
		
* Build default applications, download hex file and verify everything is ok
* Build you own app

## Building an App

Create HelloApp.cpp in apps/demos:

		#include "Platform.h"
		
		class HelloState
		{
		public:
			int OnEvent(Event* e)
			{
				switch (e->Type)
				{
					case Event::OpenApp:
						Graphics.DrawString("hello",110,100,0);
						break;
					default:
						;
				}
				return 0;
			}
		};

		INSTALL_APP(hello,HelloState);

Build, Flash and test. You should get a little app that says 'hello' and never quits.

Ok. What is going on here? Microtouch as an application framework of sorts that allows multiple
applications to be built into the firmware. An application is 'installed' with the `INSTALL_APP`
macro: the first parameter is the name that will appear in the shell, the second defines a c++ object
that maintains the applications state.

The framework (Shell.cpp) sends events to the app thorugh the OnEvent method; in this example 
the app draws "hello" on the OpenApp event which will always be the first event an app will see.
The most common events come from the touch screen: TouchDown, TouchMove and TouchUp.

Because there is only 2k of RAM in this device we really don't have the luxury of things like
memory managers. The applications state is all the heap it will ever know; its maximum size is
defined by `MAX_APP_BUFFER` in Platform.h and is set to 768 bytes by default. If the app state is larger
than `MAX_APP_BUFFER` then the application won't be visible in the ShellApp.


Lets try a slightly more complicated version:

		class HelloState
		{
		public:
			void Draw(int x, int y, int color)
			{
				Graphics.DrawString("Hello",x-10,y-6,color);
			}

			int OnEvent(Event* e)
			{
				switch (e->Type)
				{
					case Event::OpenApp:
						Draw(120,160,0);
						break;

					case Event::TouchDown:
						if (e->Touch->y > 320)
							return -1;		// Quit

					case Event::TouchMove:
						Draw(e->Touch->x,e->Touch->y,RandomBits(1));
						break;
					default:
						;
				}
				return 0;
			}
		};

		INSTALL_APP(hello,HelloState);
		
Now we can scrawl lots of randomly colored hellos over the screen and we can even quit by touching
the bottom of the screen. The Graphics.h api is fairly self explanatory: it uses 5:6:5 RGB color
and supports circles, rectangles and blitting as well as text.

Stdout is connected to the USB serial port so feel free to use printf for debugging.

## Applications

There are a number of example applications that exercise various parts of system. They don't all fit 
in the device at the same time. The `MODULES` path in the Makefile defines which apps get built
into the firmware. There are a number of predefined make targets in the Makefile:
	
		make
		make demo
		make 3d
		make pacman
		make hardware
		make wiki
		make zork
		make ebook

### Shell

The shell is the Microtouch equivalent of the Finder or Explorer. It displays a list of installed apps
and files found on the microSD card, and is responsible for lauching apps and delegating file opening
to the View app. It also has code for a simple serial console:

* ls - list files on microSD if present
* p1/p0 - turn profiling on/off
* lcd - dump lcd registers
* *appname* - launch *appname* if installed

### Off

The simplest app. It turns the Microtouch off unless it is plugged into USB. The Microtouch will
turn itself off after 5 minutes of inactivity so you don't absolutely need this one, but I find it
somehow comforting.

### Calibrate

Calibrates touchscreen and stores calibration data in EEPROM. You probably will only need to run this
app once if at all. Click in the red circles with a stylus until the application is satisfied with the
consistency of the clicks. If you have a shattered touchscreen this app may give up after half a dozen attempts.

### View

Displays files with inertial scrolling, currently IM2 files created with the MicrotouchTool are supported.

### HWTest

Exercises major hardware components including touchscreen, accelerometer, microSD and backlight.

### Accelerate

Accelerometer demo draws XYZ values and bounces a ball depending on the orientation of the device.

### View

Displays files with inertial scrolling, currently IM2 files created with the MicrotouchTool are supported.


### 3D

The classic 3D Microtouch engine with accelerometer support. Tilt the device to move the object, touch the
screen to select diffent platonic solids.

### Pacman

Omage to the greatest game ever written, demonstrates a technique for flicker free sprites at >60Fps.

### Doomed

A simple raycasting demo. Touchscreen controls speed and direction of movement.
Despite the humble 8 bittyness of the CPU it manages 25fps. Play with it until you find the red wall.

### Flip

Try and make all the dots the same color by touching to flip a pattern of 5. I find it bloody hard,
I have only managed to complete it once. Could someone please publish a deterministic solution?

### Lattice

Graphics demo generates a nice infinite mesh. Looks like it is complex 3D engine with lighting,
shading and geometry. It isn't.

### Mines

Click. Click. Boom.

### Paint

Fingerpainting with touchscreen. Press harder for a bigger brush, select one of three brushes:

* Cycle Chroma
* Cycle Luminance
* Color from accelerometer XYZ

### Wiki

A prototype wikipedia reader. Uses a prebuilt file based on the simple english corpus.
Load the "wiki.blb" (tools/MicrotouchSim/MicrotouchSim/microSD) file onto a microSD card,
follow instructions in makefile to build the demo.

### Zork/Frotz - A Z-machine interpreter for microtouch

The Z-machine was created in 1979 to play large (100k!) adventure games on small (8K!) personal computers.
Long before the Java the implementors at Infocom built a virtual machine capable of paging,
loading and saving complete runtime state that ran on a wide variety of CPUs.
Pretty sophisticated stuff for microcomputers 30 years ago.

Infocom published the most enduring works of interactive fiction; if you have not played one of
these beginning to end you are really missing one of the great joys of computer gaming.
The code is based on a cut down version of frotz, the gold standard for z-machine interpreters.

A page file on the microSD card ("p.pge") acts as its backing store. By default the app loads
"game.z5" game but will play most non v6 games: just change their name and copy to the microSD.

### eBook - an ebook reader

This is a reader for .EPB files (and .BKS bookshelves - collections of .EPB files). Standard format
epub books are converted to .EPB using the epubgrinder application in the tools directory.

The epubgrinder also contains a simulator. Because it is written using the QT framework it can be built
to run on Windows, Mac and Linux.

It supports multiple fonts, images, hyperlinks, smooth scrolling, rapid paging and bookmarks.
Select a book from the scrolling list by pressing for a second, press the back bar at the bottom of the screen to return.

There are a number of interesting .EPB books and .BKS bookshelves in the microSD folder and 
in the books.zip file in the tools directory.


## Tools

### Microtouch Tool

A simple tool to create IM2 slideshow files from jpg,png or other image files.
Add as many images as you like, right click to change background, image fit or remove and image.
Drag to reorder. when you are satisfied save the image to a microSD card and open from the Microtouch Shell.

### Microtouch Profiler

This tool is a GUI for the built-in sampling profiler. To use:

1. Connect Microtouch to USB
2. Make sure the Shell is running on the Microtouch
3. Launch MicrotouchProfiler.exe
4. File/Open the .lss listing file that was generated when the .hex file was built
5. Launch the target Microtouch app

The left panel displays a list of modules sorted by activity. The right panel shows the .lss file
which is a mixture of source and assembly plus red bars hiliting hotspots in the code. Click on
a module to move to its start in the lss file.

The built-in profiler works by sampling the PC from a timer ISR and printing it over the USB serial.
If you like hexidecimal numbers you can turn the profiler on and off by typing 'p1' and 'p0' in the console.

### Microtouch Simulator

A Win32 based simulator for developing Microtouch applications without the actual hardware.
Simulation is pixel accurate an includes a console window that emulates usb serial stdio.
The accelerometer is emulated by a series sine waves of varing period, touch pressure is selected with keys '1' thru '9'
CPU performance is not accurately emulated.

