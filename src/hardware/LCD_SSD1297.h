

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

//  SSD1297

#define LCD_ENTRY_MOD   0x11
#define LCD_RW_GRAM     0x22
#define LCD_GRAM_HOR_AD 0x4E
#define LCD_GRAM_VER_AD 0x4F

#define LCD_ENTRY_MOD_Y   0x6838
#define LCD_ENTRY_MOD_X   0x6830

      
extern const byte _SSD1298[35*3] PROGMEM;
const byte _SSD1298[35*3] = 
{
     0x28,0x00,0x06,
     0x00,0x00,0x01,    // Oscillation start
     
     0x03,0xae,0xa4,    //  Power Control 1   Line frequency and VHG,VGL voltage 
     0x0c,0x00,0x04,    //  Power Control 2   VCIX2 output voltage
     0x0d,0x00,0x0c,    //  Power Control 3   Vlcd63 voltage
     0x0e,0x28,0x00,    //  Power Control 4   VCOMA voltage VCOML=VCOMH*0.9475-VCOMA
     0x1e,0x00,0xb5,    //  Power Control 5   VCOMH voltage
     
     0x01,0x23,0x3f,    //  Driver Output
     
     0x02,0x06,0x00,    //  LCD Driver AC control
     
     0x10,0x00,0x00,    //  Sleep mode
     0x11,0x68,0x30,    //  Entry mode
     
    // 0x05,0x00,0x00,    //  Not used
    // 0x06,0x00,0x00,    //  Not used
     0x16,0xef,0x1c,
     
     0x07,0x03,0x33,    //  Display Control 1

     0x0b,0x00,0x00,    //  Frame cycle control
     0x0f,0x00,0x00,    //  Gate scan
     
     0x41,0x00,0x00,    //  Vertical scroll control 1
     0x42,0x00,0x00,    //  Vertical scroll control 2
     0x48,0x00,0x00,    //  First window start
     0x49,0x01,0x3f,    //  First window end
     0x4a,0x00,0x00,    //  Second window start
     0x4b,0x00,0x00,    //  Second window end
     
     // Don't need to init these here
     //0x44,0xEF,0x00,    //  Horizontal RAM start and end address
     //0x45,0x00,0x00,    //  Vertical RAM start address
     //0x46,0x01,0x3F,    //  Vertical RAM end address 
     //0x4e,0x00,0x00,    //  GDDRAM X
     //0x4f,0x00,0x00,    //  GDDRAM Y
     
     0x30,0x07,0x07,    //  Gamma control 1-10
     0x31,0x02,0x02,
     0x32,0x02,0x04,
     0x33,0x05,0x02,
     0x34,0x05,0x07,
     0x35,0x02,0x04,
     0x36,0x02,0x04,
     0x37,0x05,0x02,
     0x3a,0x03,0x02,
     0x3b,0x03,0x02,
     
  //   0x23,0x00,0x00,    //  Not used
  //   0x24,0x00,0x00,
     
     0x25,0x80,0x00,    //  Frame frequency
     0x26,0x70,0x00,    //  
     0x20,0xb0,0xeb,
     0x27,0x00,0x7c,
};

//  SSD1297
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
    
    byte count = sizeof(_SSD1298)/3;
    const byte* b = _SSD1298;
    while (count--)
    {
        byte r = pgm_read_byte(b++);
        int hi = pgm_read_byte(b++);
        WriteLcdReg(r,(hi << 8) | pgm_read_byte(b++));
    }
 }

void LCD_::SetBounds(int x, int y, int width, int height)
{
    WriteLcdReg(0x44,((x + width - 1) << 8) | x);   //  Horizontal RAM start and end address
    WriteLcdReg(0x45,y);                            //  Vertical RAM start address
    WriteLcdReg(0x46,y + height-1);                 //  Vertical RAM end address
	WriteLcdReg(LCD_GRAM_HOR_AD,x);  //  GDDRAM X
    WriteLcdReg(LCD_GRAM_VER_AD,y);  //  GDDRAM Y
    WriteLcdRegAddress(LCD_RW_GRAM);
}

void LCD_::Scroll(int y)
{
    while (y < 0)
        y += 320;
    while (y >= 320)
        y -= 320;
    WriteLcdReg(0x41,y);
}