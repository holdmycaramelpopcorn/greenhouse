/*
Data Acquisition System / Green House Monitoring
Used LDR,LM35,Relay,LED,LCD,Internal RTC,GPIO,PWM,Timer0 & Timer1,UART0
*/

#include <lpc214x.h>
#include <stdio.h>
#include "LCD.H"
#include "seven.h"
#define uint16_t unsigned int
	
typedef struct
{
	unsigned char sec;
  unsigned char min;
  unsigned char hour;
  unsigned char weekDay;
  unsigned char date;
  unsigned char month;
  unsigned int year;  
}rtc_t;

void SystemInit(void);//initialize CCLK and PCLK
void Board_Init(void);//initialize GPIO
void uart_init(void); 
void RTC_Init(void);
void timer1_Init(void);// generates interrupt every 1sec
void delay(int cnt);
void RTC_SetDateTime(rtc_t *rtc);
void RTC_GetDateTime(rtc_t *rtc);
void runDCMotor(unsigned int direction,unsigned int speed);
unsigned int adc(int no,int ch);// to read LDR(AD1.3),LM35(AD1.4), LMP91300(AD1.2)
void serialPrint(unsigned val);//print int on serialport
void serialPrintStr(char * buf);//print string on serialport

//global variables
rtc_t rtc; // declare a variable to store date,time
#define RELAY_ON (IO0SET = 1 << 11)
#define RELAY_OFF (IO0CLR = 1 << 11)
#define HUMRELAY_ON (IO1SET = 1 << 22)
#define HUMRELAY_OFF (IO1CLR = 1 << 22)
unsigned  int x=0;
// ISR Routine to blink LED D7 to indicate project working
__irq   void  Timer1_ISR(void)
 {
	x = ~x;//x ^ 1;
  if (x)   
    IO0SET  =  1u << 31;   //P0.31  =  1
  else   
    IO0CLR =   1u <<31;   // P0.31  = 0	 
	T1IR  =  0x01; // clear match0 interrupt, and get ready for the next interrupt
  VICVectAddr = 0x00000000 ; //End of interrupt 
 }
int main() 
{
    unsigned char msg[100];
	  unsigned int light_i,temp,hum;
  
 // initialize the peripherals & the board GPIO
	
    Board_Init();
    SystemInit();
    uart_init();
	  RTC_Init();
	  timer1_Init(); 
	  LCD_Reset();
		LCD_Init();

	// set date & time to 7th April 2020,12:05:00am 
	
		rtc.hour = 12;rtc.min =  05;rtc.sec =  00;//12:05:00hrs
    rtc.date = 07;rtc.month = 04;rtc.year = 2020;//07th April 2020
    RTC_SetDateTime(&rtc);  // comment this line after first use
	 
	
	  while(1)
    {
      RTC_GetDateTime(&rtc);//get current date & time stamp
			sprintf((char *)msg,"time:%2d:%2d:%2d  Date:%2d/%2d/%2d \x0d\xa",(uint16_t)rtc.hour,(uint16_t)rtc.min,(uint16_t)rtc.sec,(uint16_t)rtc.date,(uint16_t)rtc.month,(uint16_t)rtc.year);
			// use the time stored in the variable rtc for date & time stamping
			serialPrintStr((char*)msg);
			LCD_CmdWrite(0x80); LCD_DisplayString((char*)msg);
			delay(2000);
			
		  //Light Measurement
			light_i = adc(1,3);//readLDR();
			//FORMULA OR LOOK UP TABLE
			sprintf((char *)msg,"Light:%5d \x0d\xa",light_i);
			serialPrintStr((char*)msg);
			LCD_CmdWrite(0xC0); LCD_DisplayString((char*)msg);
			delay(2000);
			
			//Temperature Measurement
			temp    = adc(1,4);//readTemp();
			sprintf((char *)msg,"Temperature:%5d \x0d\xa",temp);
			serialPrintStr((char*)msg);
			LCD_CmdWrite(0x94); LCD_DisplayString((char*)msg);
			delay(2000);
			
			hum    = adc(1,2);
			sprintf((char *)msg,"Humidity:%5d \x0d\xa",hum);
			serialPrintStr((char*)msg);
			LCD_CmdWrite(0xD4); LCD_DisplayString((char*)msg);
			delay(2000);
							
			// to control Relay on/off based on Light intensity
			if(light_i > 300) 
				RELAY_ON;
			else
				RELAY_OFF;
			
			if(hum<300)
				HUMRELAY_ON;
			else
				HUMRELAY_OFF;
			
			// to control DC Motor Speed based on Temperature
			if(temp > 500)
			{
				//added newly
				runDCMotor(1,100);
				//while(temp > 500)
				//{
					alphadisp7SEG("-HI--");
					delay_ms(500);
				//}
				
			}
			else if((temp > 300) && (temp <=500))
			{
				runDCMotor(1,60);
				//while((temp > 300) && (temp <=500))
				//{
					alphadisp7SEG("-LE--");
					delay_ms(500);
				//}
				
			}
			else
			{
				runDCMotor(1,0);
				//while(temp<300)
				//{
					alphadisp7SEG("-LO--");
					delay_ms(500);
				//}
				
			}
    }
}
void Board_Init(void)
{
	
	IO0DIR |= 1 << 11; // RELAY IS CONNECTED TO P0.11
	IO0DIR |= 1U << 31 | 0x00FF0000; // to set P0.16 to P0.23 as o/ps ,
  IO1DIR |= 1U << 25;	              // to set P1.25 as o/p used for EN
                                    // make D7 Led (P0.31) on off for testing		
	IO0DIR |= 1U<<28|1U<<29|1U<<30; //for 7seg display
																	//P0.28 Data pin of first register
																	//P0.29 Clock pin of shift registers, make 1 to 0
	//P0.30 Strobe pin of shift registers: 1 to 0
}
unsigned int adc(int no,int ch)
{
  // adc(1,4) for temp sensor LM35, digital value will increase as temp increases
	// adc(1,3) for LDR - digival value will reduce as the light increases
	// adc(1,2) for humidity sensor - digital value will increase as humidity decreases
	unsigned int val;
	PINSEL0 |=  0x0F300000;   
	/* Select the P0_13 AD1.4 for ADC function */
  /* Select the P0_12 AD1.3 for ADC function */
	/* Select the P0_10 AD1.2 for ADC function */
  switch (no)        //select adc
    {
        case 0: AD0CR   = 0x00200600 | (1<<ch); //select channel
                AD0CR  |= (1<<24) ;            //start conversion
                while ( ( AD0GDR &  ( 1U << 31 ) ) == 0);
                val = AD0GDR;
                break;
 
        case 1: AD1CR = 0x00200600  | ( 1 << ch );       //select channel
                AD1CR |=  ( 1 << 24 ) ;                              //start conversion
                while ( ( AD1GDR & (1U << 31) ) == 0);
                val = AD1GDR;
                break;
    }
    val = (val  >>  6) & 0x03FF;         // bit 6:15 is 10 bit AD value
    return  val;
}

void RTC_Init(void)
{
   //enable clock and select external 32.768KHz
	   CCR = ((1<< 0 ) | (1<<4));//D0 - 1 enable, 0 disable
} 														// D4 - 1 external clock,0 from PCLK

// SEC,MIN,HOUR,DOW,DOM,MONTH,YEAR are defined in LPC214x.h
void RTC_SetDateTime(rtc_t *rtc)//to set date & time
{
     SEC   =  rtc->sec;       // Update sec value
     MIN   =  rtc->min;       // Update min value
     HOUR  =  rtc->hour;      // Update hour value 
     DOW   =  rtc->weekDay;   // Update day value 
     DOM   =  rtc->date;      // Update date value 
     MONTH =  rtc->month;     // Update month value
     YEAR  =  rtc->year;      // Update year value
}

void RTC_GetDateTime(rtc_t *rtc)
{
     rtc->sec     = SEC ;       // Read sec value
     rtc->min     = MIN ;       // Read min value
     rtc->hour    = HOUR;       // Read hour value 
     rtc->weekDay = DOW;      // Read day value 
     rtc->date    = DOM;       // Read date value 
     rtc->month   = MONTH;       // Read month value
     rtc->year    = YEAR;       // Read year value

}
void SystemInit(void)
{
   PLL0CON = 0x01; 
   PLL0CFG = 0x24; 
   PLL0FEED = 0xAA; 
   PLL0FEED = 0x55; 
   while( !( PLL0STAT & 0x00000400 ))
   { ; }
   PLL0CON = 0x03;
   PLL0FEED = 0xAA;  // lock the PLL registers after setting the required PLL
   PLL0FEED = 0x55;
   VPBDIV = 0x01;      // PCLK is same as CCLK i.e 60Mhz  
}
void uart_init(void)
{
 //configurations to use serial port
 PINSEL0 |= 0x00000005;  // P0.0 & P0.1 ARE CONFIGURED AS TXD0 & RXD0
 U0LCR = 0x83;   /* 8 bits, no Parity, 1 Stop bit    */
 U0DLM = 0; U0DLL = 8; // 115200 baud rate,PCLK = 15MHz
 U0LCR = 0x03;  /* DLAB = 0                         */
}
void timer1_Init()
{
	T1TCR = 0X00;
	T1MCR = 0X03;  //011 
	T1MR0 = 150000;
	T1TC  = 0X00;
	VICIntEnable = 0x0000020;  //00100000 Interrupt Souce No:5, D5=1
	VICVectAddr5 = (unsigned long)Timer1_ISR;  // set the timer ISR vector address
	VICVectCntl5 = 0x0000025;  // set the channel,D5=1,D4-D0->channelNo-5
	T1TCR = 0X01;
}
void runDCMotor(unsigned int direction,unsigned int speed)
{
	//P0.28 pin is used to control direction of motor
	if(direction==1)
		IO0CLR = 1U << 28;
	else
		IO0SET = 1U << 28;
	
	// assume it is connected to PWM6 (P0.9)
	PINSEL0 |= 2U << 18; //0X00000008;   // 10
	PWMMR0 = 10000;
	PWMMR1 = 10000;
	PWMMR6 = 10000 * speed / 100;
	PWMMCR = 0X02;
	PWMPCR = 1u << 14 | 1<<9;//0X00000800;//
	PWMTCR = 0X09; //1001;
	PWMLER = 0X43; //01000001	
}

void serialPrint(unsigned val)
{
	int i=0;
	unsigned char buf[50],ch;
	sprintf((char *)buf,"%d\x0d\xa",val);
	while((ch = buf[i++])!= '\0')
	  {
		while((U0LSR & (0x01<<5))== 0x00){}; 
    U0THR= ch;                         
	  }
}
void serialPrintStr(char * buf)
{
	int i=0;
	char ch;
	while((ch = buf[i++])!= '\0')
	  {
		  while((U0LSR & (1u<<5))== 0x00){}; 
      U0THR= ch;   
	  }
	//send new line
	//while(U0LSR & (0x01<<5)){};U0THR = 13;
	//while(U0LSR & (0x01<<5)){};U0THR = 10;	
	
}
void delay(int cnt)
{
	T0MR0 = 1000;//14926; // some arbitrary count for delay
	T0MCR = 0x0004; // set Tiimer 0 Stop after Match
	while(cnt--)
	{
		T0TC = 0X00;
	  T0TCR = 1; // start the timer (enbale)
		while(!(T0TC == T0MR0)){};// wait for the match
	  T0TCR = 2;// stop the timer		
	}
}
