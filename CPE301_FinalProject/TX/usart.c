/*
 * usart.c
 *
 * Created: 28-Apr-18 3:16:40 PM
 */ 

 #include "usart.h"

 void usart_init()
 {
	 UBRR0H = (MYUBRR) >> 8;
	 UBRR0L = MYUBRR;
	 UCSR0B |= (1 << TXEN0); // Enable transmitter
	 UCSR0C |=  (1 << UCSZ01) | (1 << UCSZ00); // Set frame: 8data, 1 stop
 }

 int USART0SendByte(char u8Data)
 {
	 //wait while previous byte is completed
	 while(!(UCSR0A&(1<<UDRE0))){};
	 // Transmit data
	 UDR0=u8Data;
	 return 0;
 }

 void USARTSendStr(char* _str)
 {
	 int thesize=strlen(_str);
	 for (uint8_t i=0; i<thesize;i++)
	 {
		 USART0SendByte(_str[i]);
	 }
 }
