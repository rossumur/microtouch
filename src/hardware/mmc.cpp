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
#include "Board.h"
#include "mmc.h"

#define SPI_PORT PORTB
#define SPI_DDR DDRB

//	
void SPI_Init()
{
	SPI_Enable();
	SPCR = 0x53;	// clock = f/128 for wake-up
	byte d;
	d = SPSR;		// clear status
	d = SPDR;		// and buffer
}

void SPI_Enable()	// TODO MISO and MOSI dir
{
	SPI_DDR = ~_BV(MISO); SPI_PORT |= _BV(MOSI); SPCR = 0x50;
}

void SPI_Disable()
{
	SPCR = 0x00;	// Don't bother changing DDR back
}

void SPI_Send(byte* data, int len)
{
	while (len--)
		SPI_ReceiveByte(*data++);
}

byte SPI_ReceiveByte(byte b)
{
	SPDR = b;
	loop_until_bit_is_set(SPSR, 7);
	return SPDR;
}

//	Read 512 bytes, hopefully quickly but not too quickly
//	Data transfer - strips the 2 CRC bytes off the end
void SPI_Receive(byte* buffer, int len)
{
	//	Unroll a bit
    SPDR = 0xFF;    // Byte 0        
    byte i = 0xFF;
    do
    {
        byte a;
        loop_until_bit_is_set(SPSR, 7);
        a = SPDR;
        SPDR = 0xFF;
        *buffer++ = a;
        loop_until_bit_is_set(SPSR, 7);
        a = SPDR;
        SPDR = 0xFF;
        *buffer++ = a;
    } while (i--);

    loop_until_bit_is_set(SPSR, 7);
    i = SPDR;                       // CRC byte 0
    SPI_ReceiveByte(0xFF);          // CRC byte 1
}

//======================================================================================================
//======================================================================================================

#define MMC_PRESENT 1
#define MMC_HIGH_DENSITY 2
#define MMC_SDHC	0x40
#define MMC_INITED	0x80
byte _mmcState=0;

byte MMC_Token()
{
	int count = 0x3FFF;	// was FFF
	byte r;
	while (count--)
	{
		r = SPI_ReceiveByte(0xFF);
		if (r != 0xFF)
			break;
	}
	return r;
}

byte MMC_Command2(byte cmd, uint32_t param)
{
	byte* b = (byte*)&param;
	byte d[7];
	d[0] = 0xFF;
	d[1] = cmd + 0x40;
	d[2] = b[3];
	d[3] = b[2];
	d[4] = b[1];
	d[5] = b[0];
	d[6] = 0xFF;
	SPI_Send(d,7);
	return 1;
}

byte MMC_Command(byte cmd, uint32_t param)
{
	byte* b = (byte*)&param;
	byte csum = 0xFF;
	if (cmd == 0)
		csum = 0x95;
	else if (cmd == 8)
		csum = 0x87;	// Avoid checksum code
	byte d[7];
	d[0] = 0xFF;
	d[1] = cmd + 0x40;
	d[2] = b[3];
	d[3] = b[2];
	d[4] = b[1];
	d[5] = b[0];
	d[6] = csum;
	SPI_Send(d,7);
	return MMC_Token();
}

byte MMC_ReadOCR(byte cmd, uint32_t param, byte* ocr)
{
	byte r = MMC_Command(cmd,param);
	byte i = 4;
	while (i--)
		*ocr++ = SPI_ReceiveByte(0xFF);
	return r;
}

//	Init at 400khz clock
//	Kick up to 6-8Mhz once we are inited (25Mhz on faster cpus)
//	SDSC
//	SDHC
//	MMC

byte MMC_Init2()
{
	byte ocr[4];
	MMC_SS_HIGH();	//
	SPI_Init();		// Init slow at first TODO
	_mmcState = 0;

	// initialize the MMC card into SPI mode by sending 80 clks
	int i;
	for (i = 0; i < 10; i++)
		SPI_ReceiveByte(0xFF);

	// Send GO_IDLE_STATE
	MMC_SS_LOW();
	if (MMC_Command(0,0) != 1)
		return GO_IDLE_TIMEOUT;
	_mmcState = MMC_PRESENT;
	MMC_SS_HIGH();

	//	Full speed
	SPI_Enable();
	MMC_SS_LOW();

	if (MMC_ReadOCR(58,0,ocr) != 1)		// Read OCR check voltages
		return READ_OCR_TIMEOUT;		// Can't read OCR
	if ((ocr[1] & 0x10) == 0)
		return BAD_VOLTAGE;				// Card does not like out voltage

	// Try CMD8
	if (MMC_ReadOCR(8,0x1AA,ocr) == 1)
	{
		//	SDHC
		_mmcState |= MMC_SDHC;
		if ((ocr[2] & 0x0F) != 1)
			return BAD_VOLTAGE;				// Card does not like out voltage
		if (ocr[3] != 0xAA)
			return BAD_PATTERN;

		//	Send ACMD41 and wait for it to come out of idle
		i = 0x7FFF;	// Wait a long time. A long, long time - some cards take 700 miliseconds or more.
		while (i--)
		{
			MMC_Command(55,0);						// SEND_APP_CMD
			if (MMC_Command(41,1L << 30) == 0)		// ACMD41 with HCS bit 30 set
				break;
		}
		if (i < 0)
			return OP_COND_TIMEOUT;

		//	Check density again
		if (MMC_ReadOCR(58,0,ocr) != 0)				// Bits[30:29]=1,0
			return READ_OCR_TIMEOUT;				// Can't read OCR
		if ((ocr[0] & 0x60) == 0x40)
			_mmcState |= MMC_HIGH_DENSITY;

	} else {

		//	SDCARD 1vXX
		//	Send SEND_OP_COND and wait for it to come out of idle
		i = 0x7FFF;
		while (i--)
		{
			if ((MMC_Command(1,0) & 1) == 0)
				break;
		}
		if (i < 0)
			return OP_COND_TIMEOUT;
	}

	//	Send SET_BLOCKLEN for 512 byte block
	if (MMC_Command(16,512) != 0)
		return SET_BLOCKLEN_TIMEOUT;

	_mmcState |= MMC_INITED;
	return 0;
}

byte MMC_Release(byte result)
{
	MMC_SS_HIGH();
	SPI_ReceiveByte(0xFF);	// CS does not release SPI until next clock....flush CS release
	SPI_Disable();
	return result;
}

byte MMC_Init()
{
	return MMC_Release(MMC_Init2());
}

byte MMC_ReadSector(byte *buffer, uint32_t sector)
{
	if (!(_mmcState & MMC_INITED))
		return MMC_NOT_INITED;
	if (!(_mmcState & MMC_HIGH_DENSITY))
		sector <<= 9;
	SPI_Enable();
	MMC_SS_LOW();
	if (MMC_Command(17,sector) != 0 || MMC_Token() != 0xFE)
		return MMC_Release(READ_FAILED);
	SPI_Receive(buffer,512);	// WARNING! Will strip 2 CRC bytes as well
	return MMC_Release(0);
}

#if 0
static char sdWaitWriteFinish(void)
{
  unsigned short count = 0xFFFF; // wait for quite some time

  while ((SPI_ReceiveByte(0xFF) == 0) && count )
    count--;

  // If count didn't run out, return success
  return (count != 0);
}
#endif

byte MMC_WriteSector(byte *buffer, uint32_t sector)
{
	if (!(_mmcState & MMC_INITED))
		return MMC_NOT_INITED;
	if (!(_mmcState & MMC_HIGH_DENSITY))
		sector <<= 9;

	u8 r = WRITE_FAILED;
	SPI_Enable();
	MMC_SS_LOW();
	if (MMC_Command2(24,sector) != 0)
	{
		u8 d = 16;
		while (d--)
			SPI_ReceiveByte(0xFF);	// pad

		SPI_ReceiveByte(0xFE);	// Send token

		SPI_Send(buffer,512);
		SPI_ReceiveByte(0xFF);	// CRC
		SPI_ReceiveByte(0xFF);	// CRC

		u8 status;
		while ((status = SPI_ReceiveByte(0xFF)) == 0xFF)
			;
		if (status == 0xE5)
			r = 0;
		else
			r = status;

		while (SPI_ReceiveByte(0xFF) == 0)
			;
		r = MMC_Release(r);
	} else {
		r = MMC_Release(WRITE_FAILED);
	}
	return r;
}