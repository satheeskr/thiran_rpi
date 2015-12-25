/*=============================================
Name: rpi_pwm_xmas_lights.c
Author: Sathees Balya
Description:
This program generates a PWM square wave of 
varying frequencies to light the xmas leds
==============================================*/
#include "stdio.h"
#include "stdlib.h"
#include "sys/mman.h"
#include "sys/types.h" 
#include "sys/stat.h"
#include "fcntl.h"
#include "unistd.h"
#include "pthread.h"
#include "rpi_pwm_xmas_lights.h"
#include "math.h"

#define NUM_THREADS 1u

/* Variable declarations */

/* Pointers to access the hardware registers */
volatile unsigned int * gpio;
volatile unsigned int * spi;
volatile unsigned int * pwm;
volatile unsigned int * pwmclk;

/*========================================================= 
Name: pwm_wave_gen
Description:
This thread outputs the PWM duty cycle through the GPIO to
generate square wave. 
*=========================================================*/
void pwm_wave_gen(unsigned int pwm_enable)
{	
unsigned char i = 0;
unsigned char j = 0;
	/* Stop the PWM */
	if (0u == pwm_enable)
	{
		/* Stop the PWM; ENAB=0 */
		PWM_CTL1 = 0x80u;
		usleep(10u);
		
		/* Tri state output port */
		INPORT(18u);
	}
	else
	{
		/* Configure GPIO18 as PWM1 output (ALT 5) to generate square wave */
		INPORT(18u);
		PORT_CONFIG(18u, 2u);
		
		/* Configure PWM clock */
		/* Stop the PWM first before changing the clock */
		PWM_CTL1 = 0x00u;
		usleep(10u);
		
		/* Stop the PWM clock; ENAB=0 */
		PWM_CLK_CTL = 0x5A000001u;
		usleep(110u);
		
		/* Wait till busy flag is cleared */
		while ((PWM_CLK_CTL & 0x80u) != 0u);
		
		/* Set the clock divider */
		PWM_CLK_DIV = (0x5A000000u) | (192u << 12u);
		usleep(10u);
		
		/* Wait till busy flag is cleared */
		while ((PWM_CLK_CTL & 0x80u) != 0u);
		usleep(10u);
		
		/* Start the clock; ENAB=1 */
		PWM_CLK_CTL = 0x5A000011u;
		usleep(10u);
		
		/*
		Example:
		PWM Base Clock Frequency = 19.2 MHz, Clock Divider = 19200,
		Range = 1024. Hence PWM clock = 19.2MHz/19200 = 1kHz or 1ms;
		PWM period: 1kHz/1024 = 0.9765Hz or 1.024s
		PWM duty cycle = 50% = 1.953125Hz  or 0.512s ON/OFF 
		*/
		
		/* Configure PWM1, Enable, PWM mode, Polarity:0, Data register used
		   Mark:Space used */
		PWM_CTL1 = 0x81u;
		usleep(10u);
				
		if (100u == pwm_enable)		
		{
		PWM_RNG1 = 1024u;
		usleep(10u);
		
		for (i = 0; i < 20; i++)
		{
		for (j = 0; j < 10; j++)
		/* Check PWM is transmitting */		
		//if ((PWM_STA1 & 0x200u) == 0x200u)
		{
			/* Set duty cycle */
			PWM_DAT1 = PWM_RNG1 * j * 0.1;
			usleep(200000);
		}	
		sleep(1);
		}
		}
		else
		{
			/* Set period of the PWM output */
			PWM_RNG1 = pwm_enable * 512u; /* 0.9765Hz; 1024 = 10 bit resolution */
			usleep(10u);
		
			/* Set duty cycle */
			PWM_DAT1 = PWM_RNG1/2;
			usleep(10u);
		}
	}
}

/*========================================================= 
Name: main
Description:
This function is the entry point of this file. It sets up the
memory map and starts three threads 
*=========================================================*/
void main(int argc, char *argv[])
{
int fp; /* File pointer to open memory map */
void * gpio_map; /* Pointer to GPIO memory */
void * spi_map; /* Pointer to SPI0 memory */
void * pwm_map; /* Pointer to PWM1 memory */
void * pwmclk_map; /* Pointer to PWM_CLK memory */

unsigned int pwm_enable = 0u;

/* Open the mem device file for writing */
if((fp = open("/dev/mem", O_RDWR|O_SYNC)) < 0)
{
	printf("Cannot open memory map for write \n");
	exit(-1);
}

/* Map peripheral memory for read/write access */
gpio_map = mmap(NULL, 0x1000u, PROT_READ|PROT_WRITE, MAP_SHARED, fp, GPIO_BASE); 
gpio = (volatile unsigned int *)gpio_map;

spi_map = mmap(NULL, 0x1000u, PROT_READ|PROT_WRITE, MAP_SHARED, fp, SPI_BASE); 
spi = (volatile unsigned int *)spi_map;

pwm_map = mmap(NULL, 0x1000u, PROT_READ|PROT_WRITE, MAP_SHARED, fp, PWM_BASE); 
pwm = (volatile unsigned int *)pwm_map;

pwmclk_map = mmap(NULL, 0x1000u, PROT_READ|PROT_WRITE, MAP_SHARED, fp, PWMCLK_BASE); 
pwmclk = (volatile unsigned int *)pwmclk_map;

if((MAP_FAILED == gpio_map) || 
	(MAP_FAILED == spi_map) || 
	(MAP_FAILED == pwm_map) ||
	(MAP_FAILED == pwmclk_map))
{
	printf("Memory map failed\n");
	exit(-1);
}

/* Close the mem file */
close(fp);


pwm_enable = atoi(argv[1]);

/* Generate PWM */
pwm_wave_gen(pwm_enable);

}