#include "seven.h"
unsigned char getAlphaCode(unsigned char alphachar)
{
	switch(alphachar)
	{
		case '-': return 0xBF;
		case 'H': return 0x89;
		case 'I': return 0xF9;
		case 'L': return 0xC7;
		case 'O': return 0xC0;
		case 'E': return 0x86;
		case ' ': return 0xFF;
		default: break;
	}
	return 0xFF;
}
void alphadisp7SEG(char *buf)
{
	unsigned char i,j;
	unsigned char seg7_data, temp=0;
	for(i=0;i<5;i++)
	{
		seg7_data=getAlphaCode(*(buf+i));
		for(j=0;j<8;j++)
		{
			temp=seg7_data&0x80;
			if(temp==0x80)
				IOSET0|=1<<28;
			else
				IOCLR0|=1<<28;
			IOSET0|=1<<29;
			delay_ms(1);
			IOCLR0|=1<<29;
			seg7_data=seg7_data<<1;
		}
	}
	IOSET0|=1<<30;
	delay_ms(1);
	IOCLR0|=1<<30;
	return;
}
	