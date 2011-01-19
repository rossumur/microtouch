

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

//  HX8347

//	TODO! BAD
#define LCD_ENTRY_MOD   0x11
#define LCD_RW_GRAM     0x22
#define LCD_GRAM_HOR_AD 0x4E
#define LCD_GRAM_VER_AD 0x4F

#define LCD_ENTRY_MOD_Y   0x6838
#define LCD_ENTRY_MOD_X   0x6830

static void LCDReg(byte reg, byte dat)
{
	DATAOUT;
	RD1;
	RS0;
	DATALPORT = reg;
	CS0;
	WR0;
	WR1;
	RS1;
	DATALPORT = dat;
	WR0;
	WR1;
	CS1;
}

void delay(ushort ms);

extern const byte _HX8347Init[] PROGMEM;
const byte _HX8347Init[] =
{  
    0x46,0x95,   //  Gamma
    0x47,0x51,   //  
    0x48,0x00,   //  
    0x49,0x36,   //  
    0x4A,0x11,   //  
    0x4B,0x66,   //  
    0x4C,0x14,   //  
    0x4D,0x77,   //  
    0x4E,0x13,   //  
    0x4F,0x4C,   //  
    0x50,0x46,   //  
    0x51,0x46,   //                  

	0x02,0x00,   // Column address start2  
    0x03,0x00,   // Column address start1  
    0x04,0x00,   // Column address end2  
    0x05,0xEF,   // Column address end1		240-1
    0x06,0x00,   // Row address start2  
    0x07,0x00,   // Row address start1  
    0x08,0x01,   // Row address end2  
    0x09,0x3F,   // Row address end1		320-1
    0x90,0x7F,   // SAP=0111 1111          

    0x01,0x06,   // IDMON=0, INVON=1, NORON=1, PTLON=0
    0x16,0x80,   // MY=1, MX=0, MV=0, ML=?, BGR=0, TEON
    0x23,0x95,   // N_DC=1001 0101
    0x24,0x95,   // P_DC=1001 0101
    0x25,0xFF,   // I_DC=1111 1111
    0x27,0x06,   // N_BP=0000 0110
    0x28,0x06,   // N_FP=0000 0110
    0x29,0x06,   // P_BP=0000 0110
    0x2A,0x06,   // P_FP=0000 0110
    0x2C,0x06,   // I_BP=0000 0110
    0x2D,0x06,   // I_FP=0000 0110
    0x3A,0x01,   // N_RTN=0000, N_NW=001          
    0x3B,0x01,   // P_RTN=0000, P_NW=001  
    0x3C,0xF0,   // I_RTN=1111, I_NW=000  
    0x3D,0x00,   // DIV=00  
    0xFF,20,  

	//	BUG was 0x10
    0x70,0xA6,   // SS=0,GS=0 CSEL=110   
    0x19,0x49,   // OSCADJ=10 0000, OSD_EN=1 //60Hz  
    0x93,0x0C,   // RADJ=1100,  
    0xFF,10,  

    0x20,0x40,   // BT=0100
    0x1D,0x07,   // VC1=111  
    0x1E,0x00,   // VC3=000  
    0x1F,0x04,   // VRH=0100 
    0x44,0x4D,   // VCM=101 0000 
    0x45,0x11,   // VDV=1 0001 
    0xFF,10,  
     
    0x1C,0x04,	// AP=100  
    0xFF,20,

    0x43,0x80,	// VCOMG=1  
    0xFF,5,

    0x1B,0x18,	// GASENB=0, PON=1, DK=1, XDK=0, DDVDH_TRI=0, STB=0  
    0xFF,40,  
                             
    0x1B,0x10,	// GASENB=0, PON=1, DK=0, XDK=0, DDVDH_TRI=0, STB=0  
    0xFF,40,                                  
     
    0x26,0x04,	//GON=0, DTE=0, D=01  
    0xFF,40,

    0x26,0x24,	//GON=1, DTE=0, D=01  
    0x26,0x2C,	//GON=1, DTE=0, D=11  
    0xFF,40,  
             
    0x26,0x3C,	// GON=1, DTE=1, D=11          
    0x35,0x38,	// EQS=38h          
    0x36,0x78,	// EQP=78h  
    0x3E,0x38,	// SON=38h
    0x40,0x0F,	// GDON=0Fh  
    0x41,0xF0,	// GDOFF  
    0x57,0x02,	// Test mode='1'  
    0x56,0x84,	// set Rpulse='1000',spulse='0100'  
    0x57,0x00,	// Test mode= '0'

	0x18,0x02	// VSCROLL ON
};

void LCD_::Init()
{
    CS1;
    RS1;
    WR1;
    RD1;
    RESET1;
    CONTROLDDR |= _BV(CS) | _BV(RS) | _BV(WR) | _BV(RD);

    CS0;
    RESET0;
    delay(1);
    RESET1;
    delay(120);
    
    int count = sizeof(_HX8347Init)/2;
    const byte* b = _HX8347Init;
    while (count--)
    {
        byte reg = pgm_read_byte(b++);
        byte v = pgm_read_byte(b++);
		if (reg == 0xFF)
			delay(v);
		else
			LCDReg(reg,v);
    }
}

void LCD_::SetBounds(int x, int y, int width, int height)
{
	int r = x + width - 1;
	int b = y + height - 1;

	LCDReg(0x2,x >> 8);		// Column address start2  
    LCDReg(0x03,x);			// Column address start1  
    LCDReg(0x04,r >> 8);	// Column address end2  
    LCDReg(0x05,r);			// Column address end1		240-1

    LCDReg(0x06,y>>8);	// Row address start2  
    LCDReg(0x07,y);		// Row address start1  
    LCDReg(0x08,b>>8);	// Row address end2  
    LCDReg(0x09,b);		// Row address end1		320-1

	DATAOUT;
	RD1;
	RS0;
	DATALPORT = 0x22;
	CS0;
	WR0;
	WR1;
	RS1;
}

void LCD_::Scroll(int y)
{
    while (y < 0)
        y += 320;
    while (y >= 320)
        y -= 320;
	LCDReg(0x14,y>>8);
	LCDReg(0x15,y);
}