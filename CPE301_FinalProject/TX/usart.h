/*
 * usart.h
 *
 * Created: 28-Apr-18 3:17:11 PM
 */ 


#ifndef USART_H_
#define USART_H_
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define FOSC 16000000 // Clock frequency = Oscillator freq
#define BAUD 9600
#define MYUBRR FOSC/16/BAUD - 1

void usart_init();
int USART0SendByte(char u8Data);
void USARTSendStr(char* _str);

#endif /* USART_H_ */