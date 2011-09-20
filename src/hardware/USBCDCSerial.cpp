

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
#include "USBCDCDesc.h"

#ifndef DISABLE_USB

//==================================================================
//==================================================================
//	Mini CDC Serial

void	USBInit(void);
u8		USBGetConfiguration(void);
int16_t USBGetChar();
int8_t	USBPutChar(u8 c);

//==================================================================
//==================================================================

typedef struct
{
	u8	lineState;
	u8	bCharFormat;
	u8 	bDataBits;
	u8 	bParityType;
	u8	dwDTERate[4];
} LineInfo;

//	9 bytes of RAM
static volatile u8 _usbConfiguration = 0;
static volatile LineInfo _usbLineInfo = {0x00, 0x00, 0xE1, 0x00, { 0x00, 0x00, 0x00, 0x08 }};

//	Any frame counting?
u8 USBConnected()
{
	u8 f = UDFNUML;
	delay(3);
	return f != UDFNUML;
}

static inline void WaitIN(void)
{
	while (!(UEINTX & (1<<TXINI)));
}

static inline void ClearIN(void)
{
	UEINTX = ~(1<<TXINI);
}

static inline void WaitOUT(void)
{
	while (!(UEINTX & (1<<RXOUTI)))
		;
}

static inline u8 WaitForINOrOUT()
{
	u8 n;
	do {
		n = UEINTX;
	} while (!(n & ((1<<TXINI)|(1<<RXOUTI))));	// Wait for IN or abort on OUT
	return (n & (1<<RXOUTI)) == 0;
}


static inline void ClearOUT(void)
{
	UEINTX = ~(1<<RXOUTI);
}

static
void Send(volatile const u8* data, uint8_t count)
{
	while (count--)
		UEDATX = *data++;
}

static
void Recv(volatile u8* data, uint8_t count)
{
	while (count--)
		*data++ = UEDATX;
}

static inline u8 Recv8()
{
	return UEDATX;
}

static inline void Send8(u8 d)
{
	UEDATX = d;
}

static
void SendPGM(const u8* data, uint8_t count)
{
	while (count--)
		UEDATX = pgm_read_byte(data++);
}

static inline void SetEP(u8 ep)
{
	UENUM = ep;
}

static inline u8 ReceivedSetupInt()
{
	return UEINTX & (1<<RXSTPI);
}

static inline void ClearSetupInt()
{
	UEINTX = ~((1<<RXSTPI) | (1<<RXOUTI) | (1<<TXINI));
}

static inline void Stall()
{
	UECONX = (1<<STALLRQ) | (1<<EPEN);
}

static inline void ResetFifos()
{
	UERST = 0x7E;
	UERST = 0;
}

static inline u8 ReadWriteAllowed()
{
	return UEINTX & (1<<RWAL);
}

static inline u8 FifoFree()
{
	return UEINTX & (1<<FIFOCON);
}

static inline void ReleaseRX()
{
	UEINTX = 0x6B;	// FIFOCON=0 NAKINI=1 RWAL=1 NAKOUTI=0 RXSTPI=1 RXOUTI=0 STALLEDI=1 TXINI=1
}

static inline void ReleaseTX()
{
	UEINTX = 0x3A;	// FIFOCON=0 NAKINI=0 RWAL=1 NAKOUTI=1 RXSTPI=1 RXOUTI=0 STALLEDI=1 TXINI=0
}

static inline u8 FrameNumber()
{
	return UDFNUML;
}

//==================================================================
//==================================================================

/*
• 6-4 - EPSIZE2:0 - Endpoint Size Bits
Set this bit according to the endpoint size:
000b: 8 bytes100b: 128 bytes
001b: 16 bytes101b: 256 bytes
010b: 32 bytes110b: 512 bytes
011b: 64 bytes111b: Reserved. Do not use th
• 3-2 - EPBK1:0 - Endpoint Bank Bits
Set this field according to the endpoint size:
00b: One bank
01b: Double bank
1xb: Reserved. Do not use this configuration.
• 1 - ALLOC - Endpoint Allocation Bit
Set this bit to allocate the endpoint memory.
Clear to free the endpoint memory.
See Section 22.6, page 267 for more details.
*/

static void InitEP(uint8_t index, uint8_t type, uint8_t size)
{
	UENUM = index;
	UECONX = 1;
	UECFG0X = type;
	UECFG1X = size == 16 ? (0x10 | 0x2) : (0x30 | 0x6);	// Single buffered 16 or Double buffered 64 
}

//	API
void USBInit(void)
{
	UHWCON = 0x01;						// power internal reg (don't need this?)
	USBCON = (1<<USBE)|(1<<FRZCLK);		// clock frozen, usb enabled
	PLLCSR = 0x12;						// Need 16 MHz xtal
	while (!(PLLCSR & (1<<PLOCK)))		// wait for lock pll
		;
	USBCON = ((1<<USBE)|(1<<OTGPADE));	// start USB clock

	UDCON = 0;							// enable attach resistor
	_usbConfiguration = 0;
	UDIEN = (1<<EORSTE)|(1<<SOFE);		// Enable interrupts
}

#define FifoByteCount()   ((u8)UEBCLX)

u8 USBGetConfiguration(void)
{
	return _usbConfiguration;
}

//	non-blocking get
int16_t USBGetChar()
{
	int c = -1;
	if (_usbConfiguration)
	{
		cli();
		SetEP(CDC_RX_ENDPOINT);
		if (ReadWriteAllowed())
		{
			c = Recv8();
			if (!ReadWriteAllowed())	// release empty buffer
				ReleaseRX();
		}
		sei();
	}
	return c;
}

//	Blocking put
int8_t USBPutChar(u8 c)
{
	cli();
	SetEP(CDC_TX_ENDPOINT);
	while (!ReadWriteAllowed())
		;						// Blocking with ints turned off TODO
	Send8(c);
	if (!ReadWriteAllowed())	// FIFO got full, release it
		ReleaseTX();
	sei();
	return c;
}

//	USB ISR
ISR(USB_GEN_vect)
{
	u8 udint = UDINT;
	UDINT = 0;

	//	End of Reset
	if (udint & (1<<EORSTI))
	{
		InitEP(0,EP_TYPE_CONTROL,16);	// init ep0
		UEIENX = 1 << RXSTPE;
		_usbConfiguration = 0;			// not configured yet
		_usbLineInfo.lineState = 0;
	}

	//	Start of Frame
	if (udint & (1<<SOFI))
	{
		SetEP(CDC_TX_ENDPOINT);			// Send a tx frame if found
		if (FifoByteCount())
			ReleaseTX();
	}
}

//	USB Endpoint ISR
ISR(USB_COM_vect)
{
	u8 n;
    SetEP(0);
    if (ReceivedSetupInt())
	{
		// SETUP	
		u8 bmRequestType = Recv8();
		u8 bRequest = Recv8();
		u8 wValueL = Recv8();
		u8 wValueH = Recv8();
		u16 wIndex = Recv8();
		wIndex |= Recv8() << 8;
		u16 wLength = Recv8();
		wLength |= Recv8() << 8;

		ClearSetupInt();

		switch (bRequest)
		{
			case SET_ADDRESS:
				ClearIN();
				WaitIN();
				UDADDR = wValueL | (1<<ADDEN);
				return;

		//	Setup Endpoints
			case SET_CONFIGURATION:
				ClearIN();

				_usbConfiguration = wValueL;
				_usbLineInfo.lineState = 0;

				InitEP(CDC_ACM_ENDPOINT,EP_TYPE_INTERRUPT_IN,16);
				InitEP(CDC_RX_ENDPOINT,EP_TYPE_BULK_OUT,64);
				InitEP(CDC_TX_ENDPOINT,EP_TYPE_BULK_IN,64);
				ResetFifos();
				return;
		
			case GET_CONFIGURATION:
				if (bmRequestType == 0x80)
				{
					WaitIN();
					Send8(_usbConfiguration);
					ClearIN();
				} else
					Stall();
				return;

			case GET_STATUS:
				WaitIN();
				Send8(0);		// All good as far as I know
				ClearIN();
				return;

			case GET_DESCRIPTOR:
			{
				const uint8_t* desc_addr = 0;
				uint8_t desc_length;
				switch (wValueH)
				{
					case 0x01: desc_addr = USB_DeviceDescriptor;
						break;
					case 0x02:
						desc_addr = USB_ConfigDescriptor;
						desc_length = sizeof(USB_ConfigDescriptor);
						break;
					case 0x03:
						switch (wValueL)
						{
							case 0x00: desc_addr = (const uint8_t *)&DESC_LANGUAGE;			break;
							#ifdef DESC_MANUFACTURER
							case 0x01: desc_addr = (const uint8_t *)&DESC_MANUFACTURER;		break;
							#endif
							#ifdef DESC_PRODUCT
							case 0x02: desc_addr = (const uint8_t *)&DESC_PRODUCT;			break;
							#endif
							#ifdef DESC_SERIAL_NUMBER
							case 0x03: desc_addr = (const uint8_t *)&DESC_SERIAL_NUMBER;	break;
							#endif
						}
						break;
				}
				if (desc_addr == 0)
				{
					Stall();
					return;
				}
				if (desc_length == 0)
					desc_length = pgm_read_byte(desc_addr);

				int len = wLength;
				if (len > desc_length)
					len = desc_length;

				// Send descriptor
				do {
					if (!WaitForINOrOUT())
						return;
					n = len;
					if (n > 16)
						n = 16;
					SendPGM(desc_addr,n);
					desc_addr += n;
					len -= n;
					ClearIN();
				} while (len || n == 16);
				return;
			}

			default:
				//	CDC Requests
				if ((bmRequestType & 0x7F) == 0x21)
				{
					switch (bRequest)
					{
						case CDC_GET_LINE_CODING:
							WaitIN();
							Send(&_usbLineInfo.bCharFormat,7);
							ClearIN();
							return;
						case CDC_SET_LINE_CODING:
							WaitOUT();
							Recv(&_usbLineInfo.bCharFormat,7);
							ClearOUT();
							ClearIN();
							return;
						case CDC_SET_CONTROL_LINE_STATE:
							_usbLineInfo.lineState = wValueL;
							WaitIN();
							ClearIN();
							return;
					}
				}
			}
	}
	Stall();
}

#endif