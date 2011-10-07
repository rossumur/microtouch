
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

// Touchscreen
// Accelerometer
// Backlight

#include "Platform.h"
#include "Board.h"
#include "mmc.h"

#ifdef DISABLE_USB
#define USBConnected() 0
#define USBInit()
#define USBPutChar(_c)
#else
u8 USBConnected();
void USBInit();
void USBPutChar(u8 c);
#endif

//================================================================
//================================================================
//	Backlight

u8 _blTarget = 0;
signed char _blStep = 0;

static
void BacklightTask()
{
	int bl = OCR3A;
	bl += _blStep;
	if ((_blStep < 0 && bl <= _blTarget) || (_blStep > 0 && bl >= _blTarget))
	{
		bl = _blTarget;
		_blStep = 0;	// Done
	}
	OCR3A = bl;
}

#define BACKLIGHT_PWM 128
static
void Backlight_Init()
{
	BACKLIGHT1;
	ICR3 = BACKLIGHT_PWM-1;
	OCR3A = BACKLIGHT_PWM-1;	// duty cycle
	TCCR3A = (1<< WGM31);
	TCCR3B = (1 << WGM33) | (1<<WGM32) | (1 << CS30);
	TCCR3A |= 2 << 6;  // enable PWM on port C6
}

//	Reflects actual value, rather than the current fade
u8 Hardware_::GetBacklight()
{
	return OCR3A<<1;
}

//	Backlight is from 0..255, linear fade interpolation
void Hardware_::SetBacklight(u8 level, int ms)
{
	_blStep = 0;
	_blTarget = level>>1;
	if (ms == 0)
	{
		OCR3A = _blTarget;
		return;
	}
	int delta = _blTarget - OCR3A;
	_blStep = delta*33/ms;
	if (_blStep == 0)
		_blStep = delta<0 ? -1 : 1;
}

//================================================================
//================================================================
//  Sampling profiler

#ifndef DISABLE_PROFILER
ROMSTRING(S_profprint);
const char S_profprint[] = "%04X\n";
void Sample(u8* stack)
{
    u8* b = stack + 18;   // THIS OFFSET MAY CHANGE IF COMPILER ELECTS TO SAVE MORE REGISTERS
    u8 hi = b[0];
    u8 lo = b[1]; 
	printf_P(S_profprint,(((u16)hi << 8) | lo)<<1);
}
#endif

//================================================================
//================================================================
//	Timers

u8 _sampler = 0;
u16 _perfhi = 0;
u16 _perf = 0;
u16 _powerOffCount;

#define POWER_OFF_COUNT ((int)(5*60*30.5))	// 5 minutes

ISR(TIMER1_OVF_vect)	// (clk div 8)/65536 ~30.5hz
{
#ifndef DISABLE_PROFILER
	if (_sampler)
	{
		u8 mark = 0x69;
		Sample(&mark);
	}
#endif

    if (++_perf == 0)	// rollover perf counter every 35 minutes or so
      ++_perfhi;

	if (!--_powerOffCount)
	{
		if (!USBConnected())
			Hardware.PowerOff();	// Only power off if not connected to USB
		_powerOffCount = POWER_OFF_COUNT;
	}

    if (_blStep)
      BacklightTask();
}

void Hardware_::Profile(bool on)
{
	if (on)
		_sampler |= 1;
	else
		_sampler &= ~1;
}

ulong Hardware_::GetPerfCounter()
{
    ushort a = TCNT1;
    ulong p = _perf;
    ushort b = TCNT1;
    if (b < a)
        p++;    // rolled over
    return (p << 16) + b;
}

//	Milliseconds
ulong Hardware_::GetTicks()
{
	ulong t = GetPerfCounter();
	return t/2000;
}

//  12/8 = 1.5 mhz ticks, ~22hz rollover at 12
//  16/8 = 2 mhz ticks, ~30hz rollover at 16
void Timer_Init()
{
    TCCR1A = 0;
    TCCR1B = 2;           // prescaler to divide clock by 8 (starts timer counting)
    TCNT1 = 0;            // clear timer1 counter
    TIFR1 |= 1<<TOV1;     // clear the overflow flag
    TIMSK1 |= 1<<TOIE1;   // enable timer1 overflow interrupts
}

void Hardware_::SetLed(bool on)
{
	if (on)
		LED1;
	else
		LED0;
}

void Hardware_::PowerOff()
{
	MCUSR = 0;
	TCCR3A = 0;
	SetLed(0);
    BACKLIGHT0;
	DDRF |= 1;
    POWER0;
    for (;;)
		;
}

//================================================================
//================================================================
//	Accelerometer

#ifdef BOARD2
void WriteAcc(u8 r, u8 v)
{
    SPI_ReceiveByte(0x80 | (r << 1));
    SPI_ReceiveByte(v);
}

u8 ReadAcc(u8 r)
{
    SPI_ReceiveByte(r << 1);
    return SPI_ReceiveByte(0);
}

void Accelerometer_Init()
{
	SPI_Enable();
    SPCR = 0x50;
    AACCS_SS_LOW();
	WriteAcc(0x0D,0x80);    // Disable I2C
	WriteAcc(0x16,0x05);    // Init 2G
    AACCS_SS_HIGH();
    SPI_Disable();
}

void Hardware_::GetAccelerometer(signed char* xyz)
{
    SPI_Enable();
    SPCR = 0x50;
    AACCS_SS_LOW();
    for (u8 i = 0; i < 3; i++)
        xyz[i] = ReadAcc(i+6);
    AACCS_SS_HIGH();
    SPI_Disable();
}
#endif

//================================================================
//================================================================
//	Battery

int ReadADC(uint8_t ch);
int Hardware_::GetBatteryMillivolts()
{
	DDRF &= 0x7F;
    PORTF &= 0x7F;
	long d = ReadADC(7);
	d *= 1652;
	return d >> 8;	//Battery divider scaled to millivolts from reference
}

//====================================================================
//====================================================================
//  Touch stuff

void ADC_Init()
{
    ADMUX = 1<<REFS0;
	ADCSRA = (1 << ADEN) | (6 << ADPS0); //Enable ADC, 64 pre-scale
}

int ReadADC(uint8_t ch)
{
  // Clear ADMUX's bottom 5 bits (MUX4:0)
  ADMUX &= ~ 0x1F;

  if ((ch > 8) && (ch <= 13)) {
    // see table 24-4
    ch += 24;
  }

  // MUX5 is in a different register
  if (ch & 0x20)
    ADCSRB |= 0x20;
  else
    ADCSRB &= ~0x20;
    
  ADMUX |= ch & 0x1F;
            
  ADCSRA |= 1 << ADEN;
  ADCSRA |= 1<<ADSC;
  while(!(ADCSRA & 0x10));		// throw away first conversion
  
  ADCSRA |= 1<<ADSC;
  while(!(ADCSRA & 0x10));
  int a = ((ADCL)| ((ADCH)<<8)); // 10-bit conversion;
  ADCSRA &= ~(1 << ADEN);

  return a;
}

void quicksort(int arr[], int left, int right)
{
    int i = left, j = right;
    int tmp;
    int pivot = arr[(left + right) >> 1];

    while (i <= j) {
        while (arr[i] < pivot)
            i++;
        while (arr[j] > pivot)
            j--;
        if (i <= j)
        {
            tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
            i++;
            j--;
        }
    }

    if (left < j)
        quicksort(arr, left, j);
    if (i < right)
        quicksort(arr, i, right);
}

int filter(int* d)
{
    quicksort(d,0,4);	// 5 samples
	return d[2] << 2;	// *4
}

//  BOARD1
//  PORTA2 D11 X+ 8
//  PORTA3 D10 Y+ 4
//  PORTA1 D9 X-  2
//  PORTA0 D8 Y-  1

//  BOARD2
//  DB4 PORTB4  ADC11   X+ 0x10
//  DB5 PORTB5  ADC12   Y+ 0x20
//  DB14 PORTD6 ADC9    X- 0x40
//  DB15 PORTD7 ADC10   Y- 0x80

typedef struct
{
	int x0;
	int y0;
	int x1;
	int y1;
} TouchConfig;

int mapp(long v, long a, long b, long range)
{
    b -= a;
    return (int)(((v - a)*range + (b>>1))/b);
}

#define MAX_EEPROM 512
bool Storage_Read(int key, int len, void* data)
{
	u8 hdr[2];
	int i;
	for (i = 0; i < MAX_EEPROM;)	// 
	{
		eeprom_read_block(hdr,(void*)i,2);
		if (hdr[0] == 0xFF)
			break;
		if (key == hdr[0])
		{
			len = min(hdr[1],len);
			eeprom_read_block(data,(void*)(i+2),len);
			return true;
		}
		i += 2 + hdr[1];
	}
	return false;
}

bool Storage_Write(int key, int len, void* data)
{
	u8 hdr[2];
	int i;
	for (i = 0; i < MAX_EEPROM;)	// 
	{
		eeprom_read_block(hdr,(void*)i,2);
		if (hdr[0] == 0xFF)
			break;
		if (hdr[0] == key && len <= hdr[1])
			break;
		i += 2 + hdr[1];
	}
	if (i + 2 + key > MAX_EEPROM)
		return false;

	hdr[0] = key;
	hdr[1] = len;
	eeprom_write_block(hdr,(void*)i,2);
	eeprom_write_block(data,(void*)(i+2),len);
	return true;
}

//	Read Pressure on board 2
static void ReadZ(int* z1, int* z2)
{
    DDRB = 0x10;        // XP = 0
    PORTB = 0x00;
    DDRD = 0x80;        // YM = 1
    PORTD = 0x80;
    *z1 = ReadADC(12);	// Measure z1 (Read YP ADC)
    *z2 = ReadADC(9);	// Measure z2 (Read XM ADC)
}

static int ReadX()
{
	DDRB = 0x10;        // XP = 1
    PORTB = 0x10;
    DDRD = 0x40;        // XM = 0
    PORTD = 0x00;
    return ReadADC(12);	// Measure X (Read YP ADC)
}

static int ReadY()
{
	DDRB = 0x20;        // YP = 1
    PORTB = 0x20;
    DDRD = 0x80;        // YM = 0
    PORTD = 0x00;
    return ReadADC(11);    // Measure Y (Read XP ADC)
}

void TestTouch(int* s)
{
	s[0] = ReadX();
	s[1] = ReadY();
	ReadZ(&s[2],&s[3]);
}

//	Get a single valid sample
u8 TouchSample(int* xx, int* yy)
{
	int z1,z2,x;
	ReadZ(&z1,&z2);	// left edge

	//	z2 = 0, z1 = 1
	//	As pressure increases z2->z1 (1-z2/z1)*x
	//	(z1-z2)*x/z1 measures resistance
	if (z2 < 8)
		return 0;	// not touching

	x = ReadX();
	int t = (long)z2*x/z1;
	t += abs(512-x)>>3;	// horrible hack to make it slightly more linear across x
	t -= 84;
	if (t <= 0)
		return 0;
	t += t;

	*yy = ReadY();
	*xx = x;
	if (t > 255)
		t = 255;
	return t;	// Range to 0..255
}

void slowsort(int* s, u8 k)
{
	for (u8 i = 0; i < k-1; i++)
	{
		for (u8 j = i+1; j < k; j++)
		{
			if (s[i] > s[j])
			{
				int t = s[i];
				s[i] = s[j];
				s[j] = t;
			}
		}
	}
}


int median5(int* s)
{
	for (u8 i = 0; i < 3; i++)
	{
		for (u8 j = i+1; j < 5; j++)
		{
			if (s[i] > s[j])
			{
				int t = s[i];
				s[i] = s[j];
				s[j] = t;
			}
		}
	}
	return s[2];
}

//	Get the median 0f 5 valid samples
u8 TouchOversample(int* xx, int *yy)
{
#define MEDIAN_COUNT 5
	u8 i = MEDIAN_COUNT*2;	// get 5 valid samples from 10 reads
	u8 s = 0;
	int x[MEDIAN_COUNT];
	int y[MEDIAN_COUNT];
	int z[MEDIAN_COUNT];
	while (i--)
	{
		u8 p = TouchSample(x+s,y+s);
		if (p)
		{
			z[s] = p;
			if (++s == MEDIAN_COUNT)	// got 5 valid samples?
			{
				*xx = median5(x);
				*yy = median5(y);
				return median5(z);
			}
		}
	}
	return 0;
}

typedef struct
{
	TouchData last;
	u32 changed;
} TouchState;

TouchConfig _touchConfig;
TouchState _touchState;
u8 _initFlags = 0;

void Touch_Init()
{
	TouchConfig& t = _touchConfig;
	t.x0 = 150;	// calibration defaults
	t.y0 = 110;
	t.x1 = 830;
	t.y1 = 890;
	_touchState.changed = 0;
	Storage_Read('T',sizeof(TouchConfig),&t);
	_powerOffCount = POWER_OFF_COUNT;
}

//	Called to calibrate TouchConfig
void TouchCalibrate(int* config)
{
	_touchConfig = *((TouchConfig*)config);
	Storage_Write('T',sizeof(TouchConfig),&_touchConfig);
}

void TouchLast(TouchData& t)
{
	t = _touchState.last;
}

u8 Hardware_::GetTouch(TouchData* e)
{
    // dim backlight as battery dies
	while ((OCR3A > 10) && (GetBatteryMillivolts() < 3700))
		OCR3A--;

	TouchState& st = _touchState;
	e->pressure = TouchOversample(&e->x,&e->y);

	// Debounce
	if ((e->pressure == 0) != (st.last.pressure == 0))
	{
		u32 t = GetPerfCounter();
		u32 elapsed = t - st.changed;
		if (elapsed < 10*2000L)			// 10ms debounce factor
			*e = st.last;				// copy old state
		else
			st.changed = t;	// State has been the same for 5 ms
	}
	st.last = *e;

	if (e->pressure)
	{
		TouchConfig& t = _touchConfig;
		e->x = mapp(e->x,t.x0,t.x1,240);
		e->y = mapp(e->y,t.y0,t.y1,320);
		_powerOffCount = POWER_OFF_COUNT;	// reset auto off
	}
	DATAOUT;
	return e->pressure;
}


static int usb_putchar(char c, FILE *stream)
{
	USBPutChar(c);
	return 0;
}

FILE _stdout;
void StdOutInit()
{
	_stdout.put = usb_putchar;
	_stdout.get = 0;
	_stdout.flags = _FDEV_SETUP_WRITE;
	_stdout.udata = 0;
	stdout = &_stdout;
}

void Hardware_::Init()
{
	BOARD_INIT();

	// Resetting because of a brownout power goes down.
	// Stay down buddy
	if (MCUSR == 4)
		PowerOff(); // now in bootloader

	Backlight_Init();
	Timer_Init();
	Accelerometer_Init();
	ADC_Init();
	USBInit();
	Touch_Init();
	sei();

	// Attach stdout to usb serial
#ifndef DISABLE_USB
	StdOutInit();
#endif
}

Hardware_ Hardware;
