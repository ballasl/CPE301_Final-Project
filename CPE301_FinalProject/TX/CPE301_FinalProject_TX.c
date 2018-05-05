/*
* Transmitter.c
*
* Created: 28-Apr-18 6:17:02 AM
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "nrf24l01.h"
#include "lis3dh.h"
#include "usart.h"

void setup_timer(void);
nRF24L01 *setup_rf(void);
lis3dhacceleration calcAcceleration();

volatile bool rf_interrupt = false;
volatile bool send_message = false;
char* tempTemplate = "TX: Values to send: x=%4.2f m/s^2 y=%4.2f m/s^2 z=%4.2f m/s^2\n";

int main(void)
{
	lis3dhacceleration acceleration;
	uint8_t to_address[5] = { 0x01, 0x01, 0x01, 0x01, 0x01 };
	
	char printBuffer[64];

	nRF24L01 *rf = setup_rf();
	setup_timer();
	usart_init();
	i2c_init();

	/* Is there a LIS3DH? */
	uint8_t deviceid=0;
	lis3dh_read_bytes(WHO_AM_I, 1, &deviceid);
	if (deviceid != 0x33)
	{
		USARTSendStr("No LIS3DH detected!\n");
		return -1;
	}
	else
	USARTSendStr("LIS3DH is available\n");

	setup_lis3dh();

	sei();
	while (1)
	{
		if (rf_interrupt) {
			rf_interrupt = false;
			int success = nRF24L01_transmit_success(rf);
			if (success != 0)
			{
				nRF24L01_flush_transmit_message(rf);
			}
		}

		if (send_message) {
			send_message = false;
			
			acceleration = calcAcceleration();
			sprintf(printBuffer, tempTemplate, acceleration.x_g, acceleration.y_g, acceleration.z_g);
			USARTSendStr(printBuffer);

			nRF24L01Message msg;
			memset(&msg,0,sizeof(msg));
			/* Copy the whole structure to be sent to receiver */
			memcpy(msg.data, &acceleration, sizeof(acceleration));
						
			msg.length = sizeof(acceleration) + 1;
			nRF24L01_transmit(rf, to_address, &msg);
		}
	}
}

lis3dhacceleration calcAcceleration()
{
	lis3dhacceleration accel;
	uint8_t accelvalues[6];
	int i;
	/* Read sequentially the LIS3DH registers from OUT_X_L to OUT_Z_H in one single swallop using autoincrement */
	lis3dh_read_bytes(OUT_X_L | 0x80,6,accelvalues); /* 0x80 for autoincrement */

	/* Calculate acceleration for all 3 axis */
	for (i=0;i<6;i=i+2)
	{
	    /* Second byte is the high-byte in big endian order (could have been changed in CTRL_REG4) */
		int tempValue = (((int)accelvalues[i+1])<<8) + (int)accelvalues[i];
		float tempFloat = (float) tempValue / LIS3DH_RANGE_4_G_DIVIDER * SENSORS_GRAVITY_EARTH;
		switch (i)
		{
			case 0:
			accel.x_g = tempFloat;
			break;
			case 2:
			accel.y_g = tempFloat;
			break;
			case 4:
			accel.z_g = tempFloat;
			break;
		}
	}
	return accel;
}


nRF24L01 *setup_rf(void) {

	nRF24L01 *rf = nRF24L01_init();
	
	rf->ss.port = &PORTB;
	rf->ss.pin = PINB2;
	rf->ce.port = &PORTB;
	rf->ce.pin = PINB1;

	rf->sck.port = &PORTB;
	rf->sck.pin = PINB5;
	rf->mosi.port = &PORTB;
	rf->mosi.pin = PINB3;
	rf->miso.port = &PORTB;
	rf->miso.pin = PINB4;
	EICRA |= (1<<ISC01); // Falling edge generates interrupt
	EIMSK |= (1<<INT0); // Use INT0 (PD2)
	nRF24L01_begin(rf);
	return rf;
}

// nRF24L01 interrupt
ISR(INT0_vect) {
	rf_interrupt = true;
}

// setup timer to trigger interrupt every second when at 16MHz
void setup_timer(void) {
	TCCR1A = 0;
	// set up timer with CTC mode and prescaling = 256
	TCCR1B |= (1 << WGM12)|(1 << CS12);
	
	// initialize counter
	TCNT1 = 0;
	/*
	Initialize compare value
	With prescalar = 256, the frequency is 16,000,000 Hz / 256 -> period = 0.000016 sec
	TimerCount=Requireddelay/period -1
	For example, for 1 sec delay -> TimerCount = 1/0.000016 -1 = 62499
	Let's consider this as the value for our delay.
	*/

	OCR1A = 62499; // value for 1 second delay
	// Enable compare interrupt
	TIMSK1 |= (1 << OCIE1A);
}

// each one second interrupt
ISR(TIMER1_COMPA_vect) {
	send_message = true;
}



