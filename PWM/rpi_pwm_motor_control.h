/*=============================================
Name: rpi_pwm_motor_control.h
Author: Sathees Balya
Description:
Header file for rpi_pwm_motor_control.c
==============================================*/

#ifndef RPI_PWM_MOTOR_CONTROL_H
#define RPI_PWM_MOTOR_CONTROL_H

/* Macros */
#define TRUE 1
#define FALSE 0

#define PERIPHERAL_BASE (0x20000000)
#define GPIO_BASE (PERIPHERAL_BASE + 0x200000)
#define SPI_BASE  (PERIPHERAL_BASE + 0x204000)
#define PWM_BASE  (PERIPHERAL_BASE + 0x20C000)
#define PWMCLK_BASE  (PERIPHERAL_BASE + 0x101000)

/* Configure ports: a - port number, b - input, output or alt mode */
#define INPORT(a)  *(gpio + a/10) &= ~(unsigned int)(7<<((a%10)*3))
#define PORT_CONFIG(a,b) *(gpio + a/10) |= (unsigned int)(b<<((a%10)*3))
#define SET_PORT(a) *(gpio + 7 + a/32) = (unsigned int)(1<<a)
#define CLR_PORT(a) *(gpio + 10 + a/32) = (unsigned int)(1<<a)

/* SPI0 registers */
#define SPI0_CS *spi
#define SPI0_DATA *(spi + 1u)
#define SPI0_CLK *(spi + 2)

/* PWM registers */
#define PWM_CTL1 *pwm
#define PWM_STA1 *(pwm + 1u)
#define PWM_RNG1 *(pwm + 4u)
#define PWM_DAT1 *(pwm + 5u)

/* PWM Clock registers */
#define PWM_CLK_CTL *(pwmclk + 40u)
#define PWM_CLK_DIV *(pwmclk + 41u)


/* The resolution of ADC is 10 bits and so the range
 is 0 to 1023 (2^10). The ADC reference voltage is 3.3V. 
 Hence scaling = Vref/range - 3.3V/1023
 */
#define ADC_SCALING ((float)3.3f/1023u)

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
