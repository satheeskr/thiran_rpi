/*=============================================
Name: rpi_pwm_xmas_lights.h
Author: Sathees Balya
Description:
Header file for rpi_pwm_xmas_lights.c
==============================================*/

#ifndef RPI_PWM_XMAS_LIGHTS_H
#define RPI_PWM_XMAS_LIGHTS_H

/* Macros */
#define TRUE 1u
#define FALSE 0u

#define PERIPHERAL_BASE (0x20000000u)
#define GPIO_BASE (PERIPHERAL_BASE + 0x200000u)
#define SPI_BASE  (PERIPHERAL_BASE + 0x204000u)
#define PWM_BASE  (PERIPHERAL_BASE + 0x20C000u)
#define PWMCLK_BASE  (PERIPHERAL_BASE + 0x101000u)

/* Configure ports: a - port number, b - input, output or alt mode */
#define INPORT(a)  *(gpio + a/10u) &= ~(unsigned int)(7u<<((a%10)*3u))
#define PORT_CONFIG(a,b) *(gpio + a/10u) |= (unsigned int)(b<<((a%10u)*3u))
#define SET_PORT(a) *(gpio + 7u + a/32u) = (unsigned int)(1u<<a)
#define CLR_PORT(a) *(gpio + 10u + a/32u) = (unsigned int)(1u<<a)

/* SPI0 registers */
#define SPI0_CS *spi
#define SPI0_DATA *(spi + 1u)
#define SPI0_CLK *(spi + 2u)

/* PWM registers */
#define PWM_CTL1 *pwm
#define PWM_STA1 *(pwm + 1u)
#define PWM_RNG1 *(pwm + 4u)
#define PWM_DAT1 *(pwm + 5u)

/* PWM Clock registers */
#define PWM_CLK_CTL *(pwmclk + 40u)
#define PWM_CLK_DIV *(pwmclk + 41u)

/* extern declarations */
void pwm_wave_gen(unsigned char pwm_enable);
#endif