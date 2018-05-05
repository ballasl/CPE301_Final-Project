/*
 * lis3dh.h
 *
 * Created: 28-Apr-18 6:26:42 AM
 */ 


#ifndef LIS3DH_H_
#define LIS3DH_H_
/* LIS3DH Registers we will be using */
#define CTRL_REG1 0x20
#define CTRL_REG2 0x21
#define CTRL_REG3 0x22
#define CTRL_REG4 0x23
#define CTRL_REG5 0x24
#define CTRL_REG6 0x25
#define WHO_AM_I  0x0F
#define OUT_X_L  0x28
#define OUT_X_H  0x29
#define OUT_Y_L  0x2A
#define OUT_Y_H  0x2B
#define OUT_Z_L  0x2C
#define OUT_Z_H  0x2D
/* Earth's gravity in m/s^2 (Reference Arduino's Adafruit_Cplay_Sensor.h) */
#define SENSORS_GRAVITY_EARTH  (9.80665F)  
/* Reference: Arduino's Adafruit_Lis3dh.cpp */            
#define LIS3DH_RANGE_4_G_DIVIDER 8190

typedef struct {
	float x_g;
	float y_g;
	float z_g;
} lis3dhacceleration;

void i2c_start(void);
void i2c_stop(void);
void i2c_init(void);
void i2c_write(unsigned char data);
int setup_lis3dh() ;
int lis3dh_read_bytes(uint8_t addr, int len, uint8_t *buf);


#endif /* LIS3DH_H_ */