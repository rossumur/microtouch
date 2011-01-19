
#if F_CPU == 12000000
#define BOARD1
#elif F_CPU == 16000000
#define BOARD2
#elif
#error "Not sure which PCB you are using"
#endif

//======================================================================
//======================================================================

#ifdef BOARD1  // Atmega644p

//  LCD
#define CONTROLPORT PORTB
#define CONTROLDDR DDRB

#define DATALDDR DDRC
#define DATAHDDR DDRA

#define DATALPORT PORTC
#define DATAHPORT PORTA

#define DATALPIN PINC
#define DATAHPIN PINA

#define CS 3
#define RS 2
#define WR 1
#define RD 0

//  PORTD
//  0 - RX
//  1 - TX
//  2 - D+,INT0
//  3 - D-
//  4 - LED
//  5 - POWER REGUATOR ENABLE
//  6 - LCD RESET
//  7 - LCD BACKLIGHT

#define LED0 cbi(PORTD,4)
#define LED1 sbi(PORTD,4)

#define POWER0 cbi(PORTD,5)
#define POWER1 sbi(PORTD,5)

#define RESET0 cbi(PORTD,6)
#define RESET1 sbi(PORTD,6)

#define BACKLIGHT0 cbi(PORTD,7)
#define BACKLIGHT1 sbi(PORTD,7)

// PORTB pin numbers
//
#define SS    4
#define MOSI  5
#define MISO  6
#define SCK    7

// Macros for setting slave select
#define SPI_SS_HIGH()  PORTB |= _BV(SS)
#define SPI_SS_LOW()   PORTB &= ~_BV(SS)
#define MMCS_START()
#define MMCS_STOP()

// set MISO as input
#define BOARD_INIT() PORTB = 0xFF; DDRB = ~_BV(MISO); PORTD = 0x7F; DDRD = 0xF0;

#endif

//======================================================================
//======================================================================

#ifdef BOARD2   // Atmega32U4

//  LCD
#define CONTROLPORT PORTF
#define CONTROLDDR DDRF

#define DATALDDR DDRB
#define DATAHDDR DDRD

#define DATALPORT PORTB // Remember this is Shared with SPI
#define DATAHPORT PORTD

#define DATALPIN PINB
#define DATAHPIN PIND

//  PORTF
#define CS 4
#define RS 5
#define WR 6
#define RD 7

#define POWER0 cbi(PORTF,0)
#define POWER1 sbi(PORTF,0)

#define RESET0 cbi(PORTF,1)
#define RESET1 sbi(PORTF,1)

#define LED0 cbi(PORTC,7)       // IR on his hardware
#define LED1 sbi(PORTC,7)

#define BACKLIGHT0 cbi(PORTC,6)
#define BACKLIGHT1 sbi(PORTC,6)

#define AACCS0 cbi(PORTE,2)
#define AACCS1 sbi(PORTE,2)


//  SPIPORT is the same as  DATALPORT; pay close attention please...
#define SS    6 // On Port E
#define SCK   1 // On port B
#define MOSI  2 // On port B
#define MISO  3 // On port B

#define MMC_SS_LOW()   PORTE &= ~_BV(SS) // Select MMC card
#define MMC_SS_HIGH()  PORTE |= _BV(SS)

#define AACCS_SS_LOW()   AACCS0		// Select Accelerometer
#define AACCS_SS_HIGH()  AACCS1;	// Deselect Accelerometer

// set MISO as input
#define CPU_PRESCALE(n)	(CLKPR = 0x80, CLKPR = (n))
#define DISABLE_JTAG()  MCUCR = (1 << JTD) | (1 << IVCE) | (0 << PUD); MCUCR = (1 << JTD) | (0 << IVSEL) | (0 << IVCE) | (0 << PUD);
#define BOARD_INIT() PORTE = (1<<2) | (1 << 6); DDRE = (1<<2) | (1 << 6); PORTF = 0xFF; DDRF = 0xFF; PORTC = 0xFF; DDRC = 0xFF; CPU_PRESCALE(0); DISABLE_JTAG();

#endif

//======================================================================
//======================================================================

//  BOARD1 and BOARD2

#define DATAOUT     DATALDDR = 0xFF;  DATAHDDR = 0xFF; // Output
#define DATAIN      DATALDDR = 0x00;  DATAHDDR = 0x00; // Input

#define sbi(port,bitnum)		port |= _BV(bitnum)
#define cbi(port,bitnum)		port &= ~(_BV(bitnum))

#define CS0 cbi(CONTROLPORT,CS)
#define CS1 sbi(CONTROLPORT,CS)
#define RS0 cbi(CONTROLPORT,RS)
#define RS1 sbi(CONTROLPORT,RS)
#define RD0 cbi(CONTROLPORT,RD)
#define RD1 sbi(CONTROLPORT,RD)
#define WR0 cbi(CONTROLPORT,WR)
#define WR1 sbi(CONTROLPORT,WR)

