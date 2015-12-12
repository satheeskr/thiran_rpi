/*=============================================
Name: rpi_pwm_motor_control.c
Author: Sathees Balya
Description:
This program generates a PWM square wave of 31.710 kHz
 and 32.710 kHz to control a remote bulb holder
==============================================*/
#include "stdio.h"
#include "stdlib.h"
#include "sys/mman.h"
#include "sys/types.h" 
#include "sys/stat.h"
#include "fcntl.h"
#include "unistd.h"
#include "pthread.h"
#include "rpi_pwm_bulb_holder.h"
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
void pwm_wave_gen(unsigned int pwm_period)
{	
unsigned int i = 0;

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
PWM_CLK_DIV = (0x5A000000u) | (1u << 12u);
usleep(10u);

/* Wait till busy flag is cleared */
while ((PWM_CLK_CTL & 0x80u) != 0u);
usleep(10u);

/* Start the clock; ENAB=1 */
PWM_CLK_CTL = 0x5A000011u;
usleep(10u);

/*
Example:
PWM Base Clock Frequency = 19.2 MHz, Clock Divider = 32,
Range = 1024. Hence PWM clock = 19.2MHz/32 = 600kHz or 1.7us;
PWM period: 600kHz/1024 = 585.9375Hz or 1.7ms
PWM duty cycle = (DATA/1024) * 585.9375Hz
Duty cycle 50% = 512/1024 * 585.9375Hz = 292.96875Hz or 0.85ms ON/OFF 
*/

/* Configure PWM1, Enable, PWM mode, Polarity:0, Data register used
   Mark:Space used */
PWM_CTL1 = 0x81u;
usleep(10u);

/* Set period of the PWM output */
PWM_RNG1 = pwm_period; /* 75Hz; 1024 = 10 bit resolution */
usleep(10u);

for (i = 0; i < 3; i++)
{
	/* Check PWM is transmitting */		
	if ((PWM_STA1 & 0x200u) == 0x200u)
	{
		/* Set duty cycle */
		PWM_DAT1 = PWM_RNG1/2;	
	}
		
	/* Generate the PWM pulse for 240ms */
	usleep(240000);
	
	/* Inter frame signal for 2 seconds */
	PWM_DAT1 = 0u;	
	sleep(2);
}

/* Stop the PWM; ENAB=0 */
PWM_CTL1 = 0x80u;
usleep(10u);

/* Tri state output port */
INPORT(18u);
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

unsigned int period;

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


/* Generate PWM */
if (1u == atoi(argv[1]))
{
	period = 605u;
}
else
{
	period = 587u;
}

pwm_wave_gen(period);

}