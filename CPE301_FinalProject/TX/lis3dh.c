/*
* lis3dh.c
*
* Created: 28-Apr-18 6:24:02 AM
*/
#include <avr/io.h>
#include <util/twi.h>
#include "lis3dh.h"
#include "usart.h"

//#define ISDEBUGGING
#ifdef ISDEBUGGING
inline void USARTDbgSendStr(char* _str)
{
	USARTSendStr(_str);
}
#else
inline void USARTDbgSendStr(char* _str)
{
	return;
}
#endif
/* BDIV = ((CPU_CLOCK_FREQ/I2C_BAUD)-16)/(2*prescalar) */
/* For prescaler = 1 and data rate = 100,000 */
#define BDIV (FOSC / 100000 - 16) / 2 + 1 // BDIV=73


/*********** I2C specific ***********/

inline void i2c_write(unsigned char data)
{
	TWDR = data;
	TWCR = (1<<TWINT) | (1<<TWEN); /* clear interrupt to start transmission */
	while ((TWCR & (1<<TWINT))==0); /* wait for transmission */
}

inline void i2c_init(void)
{
	TWSR=0x0; /* Set prescalar to 1 */
	TWBR=BDIV; /* Set bit rate register. See above */
	TWCR=(1<<TWEN); /* Enable the TWI module */
}

inline void i2c_start(void)
{
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN); /* send start condition */
	while ((TWCR & (1<<TWINT)) == 0) ; /* wait for transmission */
}

inline void i2c_stop(void)
{
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO); /* send stop condition */
}

/* ********** LIS3DH specific **********/

#define TWI_SLA_LIS3DH      0b00110000 /* LIS3DH_DEFAULT_ADDRESS for 5 volt //If SDO/SA0 is 3V is 0b00110010 */
#define MAX_ITER	        200
uint8_t twst;

int lis3dh_write_byte(uint8_t addr, uint8_t value)
{
	uint8_t sla, n = 0;
	int rv = 0;

	sla = TWI_SLA_LIS3DH;
	restart:
	if (n++ >= MAX_ITER)
	return -1;
	begin:
	USARTDbgSendStr("pointW0\n");
	i2c_start();
	switch (twst = TW_STATUS)
	{
		case TW_REP_START:	/* OK, but should not happen */
		case TW_START:
		break;
		case TW_MT_ARB_LOST:
		goto begin;
		default:
		return -1;		/* Not in start condition */
	}
	USARTDbgSendStr("pointW1\n");
	i2c_write(sla | TW_WRITE);
	switch (twst = TW_STATUS)
	{
		case TW_MT_SLA_ACK:
		break;
		case TW_MT_SLA_NACK:	/* Device busy writing */
		USARTDbgSendStr("point1Wnack\n");
		goto restart;
		case TW_MT_ARB_LOST:	
		USARTDbgSendStr("point1Wlost\n");
		goto begin;
		default:
		goto error;		/* must send stop condition */
	}
	USARTDbgSendStr("pointW2\n");
	i2c_write(addr);
	switch (twst = TW_STATUS)
	{
		case TW_MT_DATA_ACK:
		break;
		case TW_MT_DATA_NACK:
		goto quit;
		case TW_MT_ARB_LOST:
		goto begin;
		default:
		goto error;		/* must send stop condition */
	}
	USARTDbgSendStr("pointW3\n");
	i2c_write(value);
	switch (twst = TW_STATUS)
	{
		case TW_MT_DATA_NACK:
		goto error;		/* device write protected */
		case TW_MT_DATA_ACK:
		rv++;
		break;
		default:
		goto error;
	}
	
	quit:
	i2c_stop();
	USARTDbgSendStr("pointW5\n");
	return rv;

	error:
	USARTDbgSendStr("pointW4\n");
	rv = -1;
	goto quit;
}

int lis3dh_read_bytes(uint8_t addr, int len, uint8_t *buf)
{
	uint8_t sla, twcr, n = 0;
	int rv = 0;
	sla = TWI_SLA_LIS3DH;

	/* First cycle: Master transmitter mode */
	restart:
	if (n++ >= MAX_ITER)
	return -1;
	begin:
	USARTDbgSendStr("pointR0\n");
	i2c_start();
	switch (twst = TW_STATUS)
	{
		case TW_REP_START:		/* OK, but should not happen */
		case TW_START:
		break;
		case TW_MT_ARB_LOST:
		goto begin;
		default:
		return -1;		/* Not in start condition */
	}
	USARTDbgSendStr("pointR1\n");
	i2c_write(sla | TW_WRITE);
	switch (twst = TW_STATUS)
	{
		case TW_MT_SLA_ACK:
		break;
		case TW_MT_SLA_NACK:	/* Device busy writing */
		goto restart;
		case TW_MT_ARB_LOST:	
		goto begin;
		default:
		goto error;		/* must send stop condition */
	}
	USARTDbgSendStr("pointR2\n");
	i2c_write(addr);
	switch (twst = TW_STATUS)
	{
		case TW_MT_DATA_ACK:
		break;
		case TW_MT_DATA_NACK:
		goto quit;
		case TW_MT_ARB_LOST:
		goto begin;
		default:
		goto error;		/* must send stop condition */
	}

	/* Next cycle(s): master receiver mode */
	
	USARTDbgSendStr("pointR3\n");
	i2c_start();
	switch (twst = TW_STATUS)
	{
		case TW_START:		/* OK, but should not happen */
		case TW_REP_START:
		break;
		case TW_MT_ARB_LOST:
		goto begin;
		default:
		goto error;
	}
	
	/* send SLA+R */
	USARTDbgSendStr("pointR4\n");
	i2c_write(sla | TW_READ);
	switch (twst = TW_STATUS)
	{
		case TW_MR_SLA_ACK:
		break;
		case TW_MR_SLA_NACK:
		goto quit;
		case TW_MR_ARB_LOST:
		goto begin;
		default:
		goto error;
	}
	for (twcr = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);  len > 0;  len--)
	{
		if (len == 1)
		twcr = (1<<TWINT) | (1<<TWEN); /* send NAK this time */
		TWCR = twcr;		/* clear int to start transmission */
		while ((TWCR & (1<<TWINT)) == 0) ; /* wait for transmission */
		switch (twst = TW_STATUS)
		{
			case TW_MR_DATA_NACK:
			len = 0;		/* force end of loop */
			/* FALLTHROUGH */
			case TW_MR_DATA_ACK:
			*buf++ = TWDR;
			rv++;
			if(twst == TW_MR_DATA_NACK) goto quit;
			break;

			default:
			goto error;
		}
	}
	quit:
	
	TWCR = (1<<TWINT) | (1<<TWSTO) | (1<<TWEN); /* send stop condition */

	return rv;

	error:
	rv = -1;
	goto quit;
}

int setup_lis3dh()
{
	
	// CTRL_REG1 (0x20):
	// ODR3 ODR2 ODR1 ODR0 LPen Zen Yen Xen
	// ODR3		ODR2	ODR1	ODR0
	// 0		1		0		0		-> 50 Hz
	// CTRL_REG1: enable x,y,z@50Hz = 0x47
	USARTDbgSendStr("CTRL_REG1\n");
	if (lis3dh_write_byte(CTRL_REG1,0x47)==-1)
	return -1;

	// CTRL_REG2 (21h):
	// HPM1 HPM0 HPCF2 HPCF1 FDS HPCLICK HPIS2 HPIS1	// CTRL_REG2: no filters	USARTDbgSendStr("CTRL_REG2\n");	if (lis3dh_write_byte(CTRL_REG2,0x00)==-1)
	return -1;

	// CTRL_REG3 (22h):
	// I1_CLICK I1_AOI1 I1_AOI2 I1_DRDY1 I1_DRDY2 I1_WTM I1_OVERRUN --
	// CTRL_REG3: no interrupts
	USARTDbgSendStr("CTRL_REG3\n");
	if (lis3dh_write_byte(CTRL_REG3,0x00)==-1)
	return -1;

	// CTRL_REG4 (23h):
	// BDU BLE FS1 FS0 HR ST1 ST0 SIM
	// CTRL_REG4: BDU (Block data update), HR (High Resolution Output), 4G scale.
	USARTDbgSendStr("CTRL_REG4\n");
	if (lis3dh_write_byte(CTRL_REG4, 0x90)==-1)
	return -1;

	// CTRL_REG5 (24h)
	// BOOT FIFO_EN -- -- LIR_INT1 D4D_INT1 0 0
	// Normal mode, FIFO disabled
	USARTDbgSendStr("CTRL_REG5\n");
	if (lis3dh_write_byte(CTRL_REG5,0x00)==-1)
	return -1;

	// CTRL_REG6 (25h):
	// I2_CLICKen I2_INT1 0 BOOT_I1 0 - - H_LACTIVE -
	// All zero.
	USARTDbgSendStr("CTRL_REG6\n");
	if (lis3dh_write_byte(CTRL_REG6,0x00)==-1)
	return -1;
	return 0;
}
