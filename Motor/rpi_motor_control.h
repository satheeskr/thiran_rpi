/*=============================================
Name: rpi_motor_control.h
Author: Sathees Balya
Description:
Header file for rpi_motor_control.c
==============================================*/

#ifndef RPI_MOTOR_CONTROL_H
#define RPI_MOTOR_CONTROL_H

/* Macros */
#define TRUE 1u
#define FALSE 0u

#define PERIPHERAL_BASE (0x3F000000u)
#define GPIO_BASE (PERIPHERAL_BASE + 0x200000u)
#define SPI_BASE  (PERIPHERAL_BASE + 0x204000u)

/* Configure ports: a - port number, b - input, output or alt mode */
#define PORT_INPUT(a) *(gpio + a/10u) &= (unsigned int)~(7u<<((a%10u)*3u))
#define PORT_CONFIG(a,b) *(gpio + a/10u) |= (unsigned int)(b<<((a%10u)*3u))
#define SET_PORT(a) *(gpio + 7u + a/32u) = (unsigned int)(1u<<a)
#define CLR_PORT(a) *(gpio + 10u + a/32u) = (unsigned int)(1u<<a)

/* SPI0 registers */
#define SPI0_CS *spi
#define SPI0_DATA *(spi + 1u)
#define SPI0_CLK *(spi + 2u)

/* The resolution of ADC is 10 bits and so the range
 is 0 to 1023 (2^10). The ADC reference voltage is 3.3V. 
 Hence scaling = Vref/range - 3.3V/1023
 */
#define ADC_SCALING ((float)0.0032258f)

/* AD conversion counts to be ignore for filtering noise */
#define ADC_DIFFERENCE 4u

/* Temperature ramp rate */
#define RAMP_RATE 1.0f

/* Temperature resolution required */
#define TEMP_RES 0.25f

/* Type defintions */
/* The Thermistor datasheet provides the resistance at various
temperatures. A 10K resistor is connected between Thermistor and
ground. The AD converter measures the voltage drop across the 
10K resistor as the temperature of the thermistor changes. The 
thermistor has a negative temperature co-efficient which means 
its resistance decreases with increase in temperature */
typedef struct 
{
	int temp; /* Temperature */
	float voltage; /* Voltage measured by the ADC */
	float gradient; /* Slope needed to go from one point to the next */
}temp_voltage_t;

#endif
