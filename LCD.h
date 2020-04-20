#ifndef _LCD_H_
#define _LCD_H_

#include <lpc214x.h>
#define RS_ON  (IO0SET =  1U << 20)
#define RS_OFF (IO0CLR = 1U << 20)
#define EN_ON  (IO1SET = 1U << 25)
#define EN_OFF (IO1CLR = 1U << 25)


 void delay_ms(unsigned int j);//millisecond delay
 void delay_us(unsigned int count);//microsecond delay
 void LCD_SendCmdSignals(void);
 void LCD_SendDataSignals(void);
 void LCD_SendHigherNibble(unsigned char dataByte);
 void LCD_CmdWrite( unsigned char cmdByte);
 void LCD_DataWrite( unsigned char dataByte);
 void LCD_Reset(void);
 void LCD_Init(void);
 void LCD_DisplayString(const char *ptr_stringPointer_u8);



#endif
