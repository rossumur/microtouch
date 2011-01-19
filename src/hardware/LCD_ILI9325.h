

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

//  ILI9325 LCD driver

#define LCD_START_OSC			0x00
#define LCD_DRIV_OUT_CTRL		0x01
#define LCD_DRIV_WAV_CTRL		0x02
#define LCD_ENTRY_MOD			0x03
#define LCD_RESIZE_CTRL			0x04
#define LCD_DISP_CTRL1			0x07
#define LCD_DISP_CTRL2			0x08
#define LCD_DISP_CTRL3			0x09
#define LCD_DISP_CTRL4			0x0A
#define LCD_RGB_DISP_IF_CTRL1	0x0C
#define LCD_FRM_MARKER_POS		0x0D
#define LCD_RGB_DISP_IF_CTRL2	0x0F
#define LCD_POW_CTRL1			0x10
#define LCD_POW_CTRL2			0x11
#define LCD_POW_CTRL3			0x12
#define LCD_POW_CTRL4			0x13
#define LCD_GRAM_HOR_AD			0x20
#define LCD_GRAM_VER_AD			0x21
#define LCD_RW_GRAM				0x22
#define LCD_POW_CTRL7			0x29
#define LCD_FRM_RATE_COL_CTRL	0x2B
#define LCD_GAMMA_CTRL1			0x30
#define LCD_GAMMA_CTRL2			0x31
#define LCD_GAMMA_CTRL3			0x32
#define LCD_GAMMA_CTRL4			0x35 
#define LCD_GAMMA_CTRL5			0x36
#define LCD_GAMMA_CTRL6			0x37
#define LCD_GAMMA_CTRL7			0x38
#define LCD_GAMMA_CTRL8			0x39
#define LCD_GAMMA_CTRL9			0x3C
#define LCD_GAMMA_CTRL10			0x3D
#define LCD_HOR_START_AD			0x50
#define LCD_HOR_END_AD			0x51
#define LCD_VER_START_AD			0x52
#define LCD_VER_END_AD			0x53
#define LCD_GATE_SCAN_CTRL1		0x60
#define LCD_GATE_SCAN_CTRL2		0x61
#define LCD_GATE_SCAN_CTRL3		0x6A
#define LCD_PART_IMG1_DISP_POS	0x80
#define LCD_PART_IMG1_START_AD	0x81
#define LCD_PART_IMG1_END_AD		0x82
#define LCD_PART_IMG2_DISP_POS	0x83
#define LCD_PART_IMG2_START_AD	0x84
#define LCD_PART_IMG2_END_AD		0x85
#define LCD_PANEL_IF_CTRL1		0x90
#define LCD_PANEL_IF_CTRL2		0x92
#define LCD_PANEL_IF_CTRL3		0x93
#define LCD_PANEL_IF_CTRL4		0x95
#define LCD_PANEL_IF_CTRL5		0x97
#define LCD_PANEL_IF_CTRL6		0x98
#define LCD_DELAY 0xFF



#define LCD_ENTRY_MOD_X   0x0030
#define LCD_ENTRY_MOD_Y   0x0038


extern const byte _regIndex[60] PROGMEM;
const byte _regIndex[60] = {
    0x00E3,             // 0x3008 set the internal timing
    0x00E7,             // 0x0012 set the internal timing
    0x00EF,             // 0x1231 set the internal timing
    LCD_START_OSC,      // 1 start oscillator
    0xFF,               // Delay 50

    LCD_DRIV_OUT_CTRL,      // 0x0100				    //set SS, // SM
    LCD_DRIV_WAV_CTRL,      // 0x0700			        //set 1 line inversion
    LCD_ENTRY_MOD,          // 0x0030	            //set GRAM write direction, // BGR=0
    LCD_RESIZE_CTRL,        // 0x0000					    //no resizing
    LCD_DISP_CTRL2,         // 0x0202				    //front & back porch periods = 2
    LCD_DISP_CTRL3,         // 0x0000
    LCD_DISP_CTRL4,         // 0x0000
    LCD_RGB_DISP_IF_CTRL1,  // 0x0000		    //select system interface				
    LCD_FRM_MARKER_POS,     // 0x0000
    LCD_RGB_DISP_IF_CTRL2,  // 0x0000

    LCD_POW_CTRL1, // 0x0000
    LCD_POW_CTRL2, // 0x0007
    LCD_POW_CTRL3, // 0x0000
    LCD_POW_CTRL4, // 0x0000
    0xFF, // Delay 200

    LCD_POW_CTRL1, // 0x1690
    LCD_POW_CTRL2, // 0x0227 //0x0137, // 					
    0xFF, // Delay 50

    LCD_POW_CTRL3, // 0x001A //0x013C,
    0xFF, // Delay 50

    LCD_POW_CTRL4, // 0x1800 //0x1400,
    LCD_POW_CTRL7, // 0x002A //0x0007,
    0xFF, // Delay 50

    LCD_GAMMA_CTRL1, // 0x0007
    LCD_GAMMA_CTRL2, // 0x0605
    LCD_GAMMA_CTRL3, // 0x0106
    LCD_GAMMA_CTRL4, // 0x0206
    LCD_GAMMA_CTRL5, // 0x0808
    LCD_GAMMA_CTRL6, // 0x0007
    LCD_GAMMA_CTRL7, // 0x0201
    LCD_GAMMA_CTRL8, // 0x0007
    LCD_GAMMA_CTRL9, // 0x0602
    LCD_GAMMA_CTRL10, // 0x0808

    LCD_GRAM_HOR_AD, // 0x0000
    LCD_GRAM_VER_AD, // 0x0000
    LCD_HOR_START_AD, // 0x0000
    LCD_HOR_END_AD, // 0x00EF
    LCD_VER_START_AD, // 0x0000
    LCD_VER_END_AD, // 0x013F
    LCD_GATE_SCAN_CTRL1, // 0xA700
    LCD_GATE_SCAN_CTRL2, // 0x0001
    LCD_GATE_SCAN_CTRL3, // 0x0000

    LCD_PART_IMG1_DISP_POS, // 0x0000
    LCD_PART_IMG1_START_AD, // 0x0000
    LCD_PART_IMG1_END_AD, // 0x0000
    LCD_PART_IMG2_DISP_POS, // 0x0000
    LCD_PART_IMG2_START_AD, // 0x0000
    LCD_PART_IMG2_END_AD, // 0x0000

    LCD_PANEL_IF_CTRL1, // 0x0010
    LCD_PANEL_IF_CTRL2, // 0x0000
    LCD_PANEL_IF_CTRL3, // 0x0003
    LCD_PANEL_IF_CTRL4, // 0x0110
    LCD_PANEL_IF_CTRL5, // 0x0000
    LCD_PANEL_IF_CTRL6, // 0x0000

    LCD_DISP_CTRL1, // 0x0133
};

extern const uint16_t _regValues[60] PROGMEM;
extern const uint16_t _regValues[60] = {
    /*0x00E3*/              0x3008,     //set the internal timing
    /*0x00E7*/              0x0012,     //set the internal timing
    /*0x00EF*/              0x1231,     //set the internal timing
    /*LCD_START_OSC*/       1,          //start oscillator
    /*_delay_ms*/           50,

    /*LCD_DRIV_OUT_CTRL*/   0x0100,				    //set SS*/SM
    /*LCD_DRIV_WAV_CTRL*/   0x0700,			        //set 1 line inversion
    /*LCD_ENTRY_MOD*/       0x0030 | 0x1000,	            //set GRAM write direction*/BGR=1
    /*LCD_RESIZE_CTRL*/     0x0000,					    //no resizing
    /*LCD_DISP_CTRL2*/      0x0202,				    //front & back porch periods = 2
    /*LCD_DISP_CTRL3*/      0x0000,
    /*LCD_DISP_CTRL4*/      0x0000,
    /*LCD_RGB_DISP_IF_CTRL1*/0x0000,		    //select system interface				
    /*LCD_FRM_MARKER_POS*/  0x0000,
    /*LCD_RGB_DISP_IF_CTRL2*/0x0000,

    /*LCD_POW_CTRL1*/       0x0000,
    /*LCD_POW_CTRL2*/       0x0007,
    /*LCD_POW_CTRL3*/       0x0000,
    /*LCD_POW_CTRL4*/       0x0000,
    /*_delay_ms*/           200,

    /*LCD_POW_CTRL1*/       0x1690,
    /*LCD_POW_CTRL2*/       0x0227, //0x0137*/					
    /*_delay_ms*/           50,

    /*LCD_POW_CTRL3*/       0x001A, //0x013C,
    /*_delay_ms*/           50,

    /*LCD_POW_CTRL4*/       0x1800, //0x1400,
    /*LCD_POW_CTRL7*/       0x002A, //0x0007,
    /*_delay_ms*/           50,

    /*LCD_GAMMA_CTRL1*/     0x0000,//0x0007,
    /*LCD_GAMMA_CTRL2*/     0x0000,//0x0605,
    /*LCD_GAMMA_CTRL3*/     0x0000,//0x0106,
    /*LCD_GAMMA_CTRL4*/     0x0206,
    /*LCD_GAMMA_CTRL5*/     0x0808,
    
    /*LCD_GAMMA_CTRL6*/     0x0007,
    /*LCD_GAMMA_CTRL7*/     0x0201,
    /*LCD_GAMMA_CTRL8*/     0x0000,//0x0007,
    /*LCD_GAMMA_CTRL9*/     0x0000,//0x0602,
    /*LCD_GAMMA_CTRL10*/    0x0000,//0x0808,

    /*LCD_GRAM_HOR_AD*/     0x0000,
    /*LCD_GRAM_VER_AD*/     0x0000,
    /*LCD_HOR_START_AD*/    0x0000,
    /*LCD_HOR_END_AD*/      0x00EF,
    /*LCD_VER_START_AD*/    0x0000,
    /*LCD_VER_END_AD*/      0x013F,
    /*LCD_GATE_SCAN_CTRL1*/ 0xA700,
    /*LCD_GATE_SCAN_CTRL2*/ 0x0003, // 0x0001
    /*LCD_GATE_SCAN_CTRL3*/ 0x0000, // 0x0000,

    /*LCD_PART_IMG1_DISP_POS*/  0x0000,
    /*LCD_PART_IMG1_START_AD*/  0x0000,
    /*LCD_PART_IMG1_END_AD*/    0x0000,
    /*LCD_PART_IMG2_DISP_POS*/  0x0000,
    /*LCD_PART_IMG2_START_AD*/  0x0000,
    /*LCD_PART_IMG2_END_AD*/    0x0000,

    /*LCD_PANEL_IF_CTRL1*/  0x0010,
    /*LCD_PANEL_IF_CTRL2*/  0x0000,
    /*LCD_PANEL_IF_CTRL3*/  0x0003,
    /*LCD_PANEL_IF_CTRL4*/  0x0110,
    /*LCD_PANEL_IF_CTRL5*/  0x0000,
    /*LCD_PANEL_IF_CTRL6*/  0x0000,

    /*LCD_DISP_CTRL1*/      0x0133,
};

void LCD_::Init()
{
    CS1;
    RS1;
    WR1;
    RD1;
    RESET1;
    CONTROLDDR |= _BV(CS) | _BV(RS) | _BV(WR) | _BV(RD);

    RESET0;
    delay(1);
    RESET1;
    delay(1);

    for (byte i = 0; i < 60; i++)
    {
        byte index = pgm_read_byte(&_regIndex[i]);
        int value = pgm_read_word(&_regValues[i]);
        if (index == 0xFF)
            delay(value);
        else
            WriteLcdReg(index,value);
    }
}

void LCD_::SetBounds(int x, int y, int width, int height)
{
    ASSERT(x >= 0 && (x + width <= (int)Width()) && y >= 0 && (y + height <= (int)Height()) && width > 0 && height > 0);
    WriteLcdReg(LCD_HOR_START_AD, x);
    WriteLcdReg(LCD_HOR_END_AD, x + width-1);
    WriteLcdReg(LCD_VER_START_AD, y);
    WriteLcdReg(LCD_VER_END_AD, y + height-1);
    WriteLcdReg(LCD_GRAM_HOR_AD, x);
    WriteLcdReg(LCD_GRAM_VER_AD, y);
    WriteLcdRegAddress(LCD_RW_GRAM);
}

void LCD_::Scroll(int y)
{
    while (y < 0)
        y += 320;
    while (y >= 320)
        y -= 320;
    WriteLcdReg(LCD_GATE_SCAN_CTRL3,y);
}


void LCD_::Direction(u8 landscape, u8 dx, u8 dy)
{
	u16 v = 0x1000;	// dx and dy
	if (landscape)
		v |= 8;
	if (dx)
		v |= 0x10;
	if (dy)
		v |= 0x20;
	WriteLcdReg(LCD_ENTRY_MOD,v);
}