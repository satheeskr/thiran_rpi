/*=============================================
Name: rpi_led.c
Author: Sathees Balya
Description:
This file configures a GPIO port to control a 
LED.
==============================================*/
#include "stdio.h"
#include "stdlib.h"
#include "sys/mman.h"
#include "sys/types.h" 
#include "sys/stat.h"
#include "fcntl.h"
#include "unistd.h"

/* Pre-processor defines */
#define PERIPHERAL_BASE (0x20000000)
#define GPIO_BASE (PERIPHERAL_BASE + 0x200000)
#define INPORT(a)  *(gpio + a/10) &= ~(unsigned int)(7<<((a%10)*3))
#define OUTPORT(a) *(gpio + a/10) |= (unsigned int)(1<<((a%10)*3))
#define SET_PORT(a) *(gpio + 7 + a/32) = (unsigned int)(1<<a)
#define CLR_PORT(a) *(gpio + 10 + a/32) = (unsigned int)(1<<a)

/* Variable declarations */
volatile unsigned int * gpio;

/* Private function declarations */
static void blink(void);

/*========================================================= 
Name: main
Description:
This function is the entry point of this file. It sets up the
memory map and configures a GPIO port as output. A LED can be 
turned off/on or blinked as per user command.
*=========================================================*/
void main(void)
{
int fp; /* File pointer to open memory map */
void * gpio_map; /* Pointer to GPIO memory */
unsigned char input; /* User input */

/* Open the mem device file for writing */
if((fp = open("/dev/mem", O_RDWR|O_SYNC)) < 0)
{
	printf("Cannot open memory map for write \n");
	exit(-1);
}

/* Map peripheral memory for read/write access */
gpio_map = mmap(NULL, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, fp, GPIO_BASE); 
gpio = (volatile unsigned int *)gpio_map;

if(MAP_FAILED == gpio_map)
{
	printf("Memory map failed\n");
	exit(-1);
}

/* Close the mem file */
close(fp);

/* Configure port 17 as output */
OUTPORT(17);

/* Get user input */
printf("\nEnter 0 for LED on\n");
printf("Enter 1 for LED off\n");
printf("Enter 2 for LED blink\n");
printf("Enter q for exit\n");

/* Run until user quits */
while((input = getchar()) != 'q')
{
	switch(input)
	{
	case '0':
		printf("Turn on LED\n");
		SET_PORT(17);
		sleep(1);
		break;
	case '1':
		printf("Turn off LED\n");
		CLR_PORT(17);
		sleep(1);
		break;
	case '2':
		blink();
		break;
	default:
		printf("Enter 0, 1, 2 or q to quit\n");
		break;
	}
}

/* Configure port 17 as input to turn off outputs safely */
INPORT(17);
}

/*========================================================= 
Name: blink
Description:
This function turns off/on a LED 10 times. 500ms ON time and
500ms OFF time. Duty cycle is TON/(TON + TOFF) in % => 50%.
*=========================================================*/
static void blink(void)
{
unsigned int i = 0;

printf("Blinking LED\n");
while(i < 10)
{
	/* Turn ON LED */
	SET_PORT(17);
	
	/* Wait for 500ms */
	usleep(500000);
	
	/* Turn OFF LED */
	CLR_PORT(17);
	
	/* Wait for 500ms */
	usleep(500000);
	
	/* Increment loop counter */
	i++;
}
}
