//**************************************
// Clock 16Mhz
// Version 1.0 Janvier 2002

//**************************************

//**************************************
//            I N C L U D E
//**************************************
#include <io2313v.h>
#include <macros.h>

//**************************************
//            D E F I N E
//**************************************
#define TRUE		0x01
#define FALSE		0x00
#define ANALOG		0x01
#define DIGITAL		0x02

//**************************************
//  I N T E R R U P T   H A N D L E R
//**************************************
#pragma interrupt_handler Crossing_interrupt:2 
#pragma interrupt_handler IR_interrupt:4
#pragma interrupt_handler Degre_interrupt:5 
#pragma interrupt_handler Ticker_interrupt:7

//**************************************
//          P R O T O T Y P E
//**************************************

void Crossing_interrupt(void);
void Degre_interrupt(void);
void Time(unsigned char);
void IR_interrupt(void);
void Ticker_interrupt(void);
void Display(void);
void CopyData(int Value);
void CopyDot(void);

//**************************************
//   G L O B A L   V A R I A B L E
//**************************************

int WeelPosition;
unsigned char Pos;
unsigned int Adder;

unsigned char LatchedIrData;

unsigned char Sec;
unsigned char Min;
unsigned char Hrs;

int SecComp;
int MinComp;
int HrsComp;

unsigned char ClockStyle;

unsigned char TimeString[50];
unsigned char *TimeStringPtr;


unsigned char i;

//**************************************
//           C O N S T A N T
//**************************************
const unsigned char table[12][6] = {{ 0x3e, 0x41, 0x41, 0x41, 0x3e, 0x00 }, // 0	
  		   					        { 0x00, 0x21, 0x7f, 0x01, 0x00, 0x00  }, // 1 
	  		   					    { 0x21, 0x43, 0x45, 0x49, 0x31, 0x00  }, // 2 
									{ 0x42, 0x41, 0x51, 0x69, 0x46, 0x00  }, // 3
									{ 0x0c, 0x14, 0x24, 0x5f, 0x04, 0x00  }, // 4
									{ 0x72, 0x51, 0x51, 0x51, 0x4e, 0x00  }, // 5
									{ 0x1e, 0x29, 0x49, 0x49, 0x06, 0x00  }, // 6
									{ 0x40, 0x47, 0x48, 0x50, 0x60, 0x00  }, // 7
									{ 0x36, 0x49, 0x49, 0x49, 0x36, 0x00  }, // 8
									{ 0x30, 0x49, 0x49, 0x4a, 0x3c, 0x00  }, // 9
									{ 0x00, 0x36, 0x36, 0x00, 0x00, 0x00  }, // :
									{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  }};// space


//**************************************
//            M A I N
//**************************************
void main()
{
WDTCR = 0x0e;			// Enable WatchDog at 0.97 sec
		
PORTD = 0x0d;			// Pull up on PD2&PD3 & Led White ON
DDRD = 0x03;			// PD0-O PD1-O PD2-I PD3-I PD4-I PD5-I PD6-I PD7-I

//INT 0
MCUCR = 0x02;			// Int0 generate int on falling eadge
GIMSK = 0x40;			// Int0 enable

//Timer0
TCCR0 = 0x05;			// Timer0 / 1024 

//Timer1
TCCR1B = 0x42;			// Timer1 / 8 & Input Capture on Rising eadge
TIMSK = 0x4a; 			// int enable on Timer1 Compare Match 
	  					// int enable on Timer 1 Input Capture
						// int enable on Timer0 Overflow
PORTB = 0x00;			
DDRB = 0xff;			// PB0-7 as output

Hrs = 0;
Min = 0;
Sec = 0;
ClockStyle = ANALOG;

SEI();

while(1)
	{
	asm("WDR");
	for (i=0;i<200;i++);
	if ((LatchedIrData == 0xbb) || (LatchedIrData == 0x92)) Time(TRUE);
	if ((LatchedIrData == 0xb3) || (LatchedIrData == 0xb0)) ClockStyle = DIGITAL;
	if ((LatchedIrData == 0xb4) || (LatchedIrData == 0xb1)) ClockStyle = ANALOG;	
   
	LatchedIrData = 0;
	}
}

/**********************************************************

Name:			void Time(void)

Description:	

Input:			none

Output:			none

Misc:			

**********************************************************/	
void Time(unsigned char Fast)
{
if (Fast == FALSE) Sec++;
else Sec += 60;
if (Sec > 59)
   {
   Sec = 0;
   Min++;
   if (Min > 59)
   	  {
	  Min = 0;
	  Hrs++;
	  if (Hrs > 11)
	  	 {
		 Hrs = 0;
		 }
	  }
   }
  
if (ClockStyle == ANALOG)
   {   
   SecComp = Sec*6;
   MinComp = Min*6;
   HrsComp = (Hrs*30)+(Min/2);
   }
else
   {
   TimeStringPtr = &TimeString[0];
   CopyData(Hrs);
   CopyDot();
   CopyData(Min);
   CopyDot();
   CopyData(Sec);   
   }
}

/**********************************************************

Name:			void CopyData(int Value)

Description:	

Input:			none

Output:			none

Misc:			

**********************************************************/	

void CopyData(int Value)
{
if (Value < 10)
   {
   for (i=0;i<6;i++) *TimeStringPtr++ = table[0][i];
   for (i=0;i<6;i++) *TimeStringPtr++ = table[Value][i];
   }
else
   {
   for (i=0;i<6;i++) *TimeStringPtr++ = table[Value/10][i];	
   for (i=0;i<6;i++) *TimeStringPtr++ = table[Value-((Value/10)*10)][i];
   }	
}

/**********************************************************

Name:			void CopySpace(void)

Description:	

Input:			none

Output:			none

Misc:			

**********************************************************/	

void CopyDot(void)
{
for (i=0;i<6;i++) *TimeStringPtr++ = table[10][i];
}

/**********************************************************

Name:			void Crossing_interrupt(void

Description:	

Input:			none

Output:			none

Misc:			

**********************************************************/	
void Crossing_interrupt(void)
{
static unsigned int LastCount;
static unsigned int TotalCount;
static int Latch;
static unsigned char Lap;

Latch = TCNT1;
TotalCount = Latch - LastCount;
LastCount = Latch;
Lap++;
if (Lap > 250)
   {
   Adder = TotalCount / 378; 
   Lap = 0;
   }
   
WeelPosition = 0;
OCR1 = Latch + Adder;
TIFR |= 0x80;
Display();
}

/**********************************************************

Name:			void Degre_interrupt(void)

Description:	

Input:			none

Output:			none

Misc:			

**********************************************************/	
void Degre_interrupt(void)
{
OCR1 = TCNT1 + Adder;
Display();
}

/**********************************************************

Name:			void Display(void)

Description:	

Input:			none

Output:			none

Misc:			

**********************************************************/	

void Display(void)
{
PORTB = 0x00;

if (ClockStyle == ANALOG)
   {
   if (WeelPosition == HrsComp) PORTB = 0x80;
   if (WeelPosition == MinComp) PORTB = 0xff;
   if (WeelPosition == SecComp) PORTB |= 0x03;

   if ((WeelPosition == 0) ||
      (WeelPosition == 30) ||
	  (WeelPosition == 60) ||
	  (WeelPosition == 90) ||
	  (WeelPosition == 120) ||
	  (WeelPosition == 150) ||
	  (WeelPosition == 180) ||
	  (WeelPosition == 210) ||
	  (WeelPosition == 240) ||
	  (WeelPosition == 270) ||
	  (WeelPosition == 300) ||
	  (WeelPosition == 330)) PORTB |= 0x01;
   }
else  
   {
   Pos = ((WeelPosition-100) / 3);
   if (Pos < 49)
   	  {
	  PORTB = TimeString[48-Pos];
	  }
   }
WeelPosition++;
}

/**********************************************************

Name:			void IR_interrupt(void)

Description:	This routine is called whenever a rising edge (beginning
                of valid IR signal) is received.

                - The data codes are sent using pulse coding.
                - Each packet has 12 bits and a header.
                - The basic time period T = 550us.
                - The header length = 4T (2.2ms)
                - 0 = pulse with length T followed by space of length T.
                - 1 = pulse with length 2T followed by space of length T.
                - The last 5 bits represent the Addess.
                - The first 7 bits represent the command.
                - A packet is transmitted every 25ms while a button is down.

Input:			none		

Output:			Global variable LatchedIrData

Misc:			Sony VCR protocol

**********************************************************/	
void IR_interrupt(void)
{
static unsigned int LastCapture;
unsigned int PulseWidth;
static unsigned int IrPulseCount;
static unsigned int IrData;

PulseWidth = ICR1 - LastCapture;
LastCapture = ICR1;


if (PulseWidth > 4000) 
   {
   IrPulseCount = 0;
   IrData = 0;
   }
else
   {
   IrData = IrData >> 1;
   if (PulseWidth > 2800) IrData = IrData | 0x8000;
   IrPulseCount++;
   if (IrPulseCount == 12) LatchedIrData = ((IrData >> 4) & 0x00ff);
   }
}

/**********************************************************

Name:			void Ticker_interrupt(void)

Description:	

Input:			none

Output:			none

Misc:			

**********************************************************/	
void Ticker_interrupt(void)
{
static unsigned char Tick;
Tick++;
if (Tick > 62)
   {
   Time(FALSE);
   Tick = 0;
   }
TCNT0 = 0x04; // reload counter
}
