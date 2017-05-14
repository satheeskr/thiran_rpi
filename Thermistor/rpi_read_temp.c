/*=============================================
Name: rpi_read_temp.c
Author: Sathees Balya
Description:
This file measures temperature from a sensor. It 
configures the Serial Peripheral Unit (SPI) in 
the micro-controller to communicate with a 
8 channel Analog to Digital Converter 
(ADC - MCP3008). The ADC reads
the voltage drop across a thermistor and sends 
the voltage as counts. The software scales the
counts to voltage based on the ADC resolution.
The software then uses a look up table to convert
the voltage to temperature.
==============================================*/
#include "stdio.h"
#include "stdlib.h"
#include "sys/mman.h"
#include "sys/types.h" 
#include "sys/stat.h"
#include "fcntl.h"
#include "unistd.h"

#include "rpi_read_temp.h"

/* Variable declarations */
/* Pointers to access the hardware registers */
volatile unsigned int * gpio;
volatile unsigned int * spi;

/* Voltage = Vref * 10K/(10K + Rth) 
Rth - Thermistor resistance at a particular temperature (refer Thermistor
datasheet
Gradient (m) = (y2-y1)/(x2-x1); -35-(-40)/(0.061281337 - 0.046025105)
 */
const temp_voltage_t temp_voltage_tab[] = 
{
/* Temp Voltage		Gradient */
-40,    0.046025105,    327.7349121,
-35,    0.061281337,    259.0036998,
-30,    0.080586081,    207.6744354,
-25,    0.104662226,    168.3354637,
-20,    0.134364821,    138.2062467,
-15,    0.170542636,    115.4029453,
-10,    0.213869086,    96.86611111,
 -5,    0.265486726,    83.12587108,
  0,    0.325636471,    71.91576505,
  5,    0.395162256,    63.25606169,
 10,    0.474206064,    56.40430353,
 15,    0.562851782,    51.05105485,
 20,    0.660792952,    46.882805,
 25,    0.76744186,     43.78135031,
 30,    0.881645739,    41.39607606,
 35,    1.002430134,    39.87577413,
 40,    1.127819549,    38.80644444,
 45,    1.256664128,    38.32200957,
 50,    1.387137453,    38.63788625,
 55,    1.516544118,    38.90424242,
 60,    1.645064806,    40.26333506,
 65,    1.769247266,    41.69394144,
 70,    1.889168766,    43.72441993,
 75,    2.003521341,    46.26492469,
 80,    2.111594574,    49.31537498,
 85,    2.212982833,    52.97352807,
 90,    2.307369599,    57.20467897,
 95,    2.394775036,    62.13303571,
100,    2.475247525,    67.74849741,
105,    2.5490499,      74.29606789,
110,    2.616348212,    81.49450142,
115,    2.677702045,    89.45125541,
120,    2.73359841,     98.99641345,
125,    2.78410529,     109.0734162,
130,    2.829945974,    120.1436364,
135,    2.871562826,    133.4615561,
140,    2.909026798,    147.1203701,
145,    2.943012575,    163.9550382,
150,    2.97350874,     50.44545455,
};

/* Function declarations */
static float get_temperature(float temp_adc_v);
static void blink(unsigned char num, unsigned char rate);
inline static void spi_config(void);
static unsigned short int spi_read_adc(unsigned char channel);
static float ramp_temperature(float temp_ramp, float temp);
static float calc_diff(float temp_ramp, float temp);

void main(void)
{
int fp; /* File pointer to open memory map */
void * gpio_map; /* Pointer to GPIO memory */
void * spi_map; /* Pointer to SPI0 memory */

unsigned short int thermistor_counts; /* ADC value in counts */
unsigned short int thermistor_counts_prev; /* Previous value of ADC counts */
float temp_adc_v; /* ADC value in volts */
float temp; /* Temperature read by the thermistor */
float temp_ramp = 0.0f; /* Temperature ramped output */
static float prev_temp_ramp; /* Previous value of temperature ramped output  */

static unsigned char first_run = TRUE; /* Flag to indicate first execution */
unsigned char print = FALSE; /* Display temperature */

/* Open the mem device file for writing */
if((fp = open("/dev/mem", O_RDWR|O_SYNC)) < 0)
{
	printf("Cannot open memory map for write \n");
	exit(-1);
}

/* Map peripheral memory for read/write access */
gpio_map = mmap(NULL, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, fp, GPIO_BASE); 
gpio = (volatile unsigned int *)gpio_map;

spi_map = mmap(NULL, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, fp, SPI_BASE); 
spi = (volatile unsigned int *)spi_map;

if((MAP_FAILED == gpio_map) || (MAP_FAILED == spi_map))
{
	printf("Memory map failed\n");
	exit(-1);
}

/* Close the mem file */
close(fp);
	
/* Configure SPI and GPIO registers */
spi_config();

/* Run indefinitely */
//while(1)
{
	/* Transmit ADC read command and receive results */
	thermistor_counts = spi_read_adc(0);

	/* Ignore small change in counts */
	if (abs(thermistor_counts_prev - 
		thermistor_counts) < ADC_DIFFERENCE)
	{
		thermistor_counts = thermistor_counts_prev;
	}
	
	/* Convert counts to volts */
	temp_adc_v = thermistor_counts * ADC_SCALING;

	/* Convert volts to temperature */				
	temp = get_temperature(temp_adc_v);
		
	/* First run */
	if (TRUE == first_run)
	{
		first_run = FALSE;
		
		/* No need to ramp for the first time */
		temp_ramp = temp;
		print = TRUE;
	}	
	
	/* Look for temperature change and ramp slowly */
	if (calc_diff(temp_ramp, temp) > 0.5)
	{
		temp_ramp = ramp_temperature(temp_ramp, temp);
		print = TRUE;
	}
	
	/* Display temperature */
	if (TRUE == print)
	{	
		print = FALSE;
		//system("date");
		//fprintf (stderr, "Temperature is: %4.2f degC\n", temp_ramp);
		//printf ("Temperature is: %4.2f degC\n", temp_ramp);
		printf ("%2.2f", temp_ramp);
	}
	
	/* Blink LED if the temperature is out of range */
	if ((temp_ramp > 25.0f) || (temp_ramp < 17.0f))
	{			
		blink(2, 2.5);						
	}
	else
	{
		/* Wait before next read */			
		sleep(1);
	}

	/* Store previous temperature value */
	prev_temp_ramp = temp_ramp;
	thermistor_counts_prev = thermistor_counts;
}
}

/*========================================================= 
Name: get_temperature
Description:
This function converts voltage read from the ADC to 
temperature. It goes through a lookup table of temperature, 
voltage and gradient to find the nearest match. It interpolates
using two-point formula to calculate the temperature.
y=mx+c where y is the temperature, m is the gradient or slope,
x is the nearest lower voltage and c is the nearest lower temperature
from the lookup table. 
*=========================================================*/
static float get_temperature(float temp_adc_v)
{
unsigned char index;
float temp_low;
float temp_high;
float temp_diff; /* Change in temperature */

/* Initialise temperature to minimum value from the lookup table */
float temp = temp_voltage_tab[0].temp; 
unsigned char tab_length = sizeof(temp_voltage_tab)/sizeof(temp_voltage_tab[0]);

/* Limit the temperature to maximum value in the lookup table */
if (temp_adc_v >= temp_voltage_tab[tab_length - 1].voltage)
{
	temp = temp_voltage_tab[tab_length - 1].temp;
}
else
{
	for (index = 0; index < tab_length; index++)
	{
		/* Find lower and upper points in the table */
		if ((temp_adc_v > temp_voltage_tab[index].voltage) &&
			(temp_adc_v < temp_voltage_tab[index + 1].voltage))
		{			
			temp_low = temp_voltage_tab[index].temp;
			temp_high = temp_voltage_tab[index + 1].temp;
			
			/* y=mx + c */
			temp = temp_voltage_tab[index].gradient * 
				(temp_adc_v - temp_voltage_tab[index].voltage)
				 + temp_voltage_tab[index].temp;
			break;
		}
	}
}

/* Scale temperature to 0.xx resolution for
easy reading */
/* Eg: 20.78 - 20; 0.78 > 0.5; 20 + 1 = 21 */
/* Eg: 20.28 - 20; 0.28 < 0.5; 20 + 0.5 = 20.5 */
temp_diff = (temp - (int)temp);	
temp_diff = ((int)((temp_diff*100)/(TEMP_RES*100)) + 1) * TEMP_RES;			
temp = (int)temp + temp_diff;
	
return(temp);
}

/*========================================================= 
Name: blink
Description:
This function flashes a LED "num" number of times with a
50% duty cycle (On time = off time)
*=========================================================*/
static void blink(unsigned char num, unsigned char rate)
{
unsigned int i = 0;

while(i < num)
{
	SET_PORT(17);
	usleep(rate * 100000); /* Sleep in microseconds */
	CLR_PORT(17);
	usleep(rate * 100000);
	i++;
}
}

/*========================================================= 
Name: spi_config
Description:
This function configures the GPIO ports as SPI
*=========================================================*/
inline static void spi_config(void)
{
/* Configure GPIO17 as output port to control a LED */
PORT_CONFIG(17u, 1u);

/* Configure GPIO ports as SPI mode ALT0 */
PORT_CONFIG(7u, 4u); /* CE1 */
PORT_CONFIG(8u, 4u); /* CE0 */
PORT_CONFIG(9u, 4u); /* MISO */
PORT_CONFIG(10u, 4u); /* MOSI */
PORT_CONFIG(11u, 4u); /* SCLK */

/* Configure SPI Clock frequency
SPI speed = ABP frequency/CDIV
ABP frequency = 250MHz
CDIV = 16384
SPI speed = 15.25kHz
 */
SPI0_CLK = 256u;
}

/*========================================================= 
Name: spi_read_adc
Description:
This function sends SPI command to read ADC channel and 
returns the results
*=========================================================*/
unsigned short int spi_read_adc(unsigned char channel)
{
unsigned char result[3u]; /* Data received from SPI0 */
unsigned short int thermistor_counts = 0; /* ADC value in counts */

SPI0_CS = 0x10; /* Clear Tx FIFO */
SPI0_CS = 0x20; /* Clear Rx FIFO */
SPI0_CS = 0x80; /* TA active */

/* Transmit ADC CH0 read command if TX FIFO is not full */	
if((SPI0_CS & 0x40000) != 0u)
{
	/* Send Start Bit in byte 1, 
	   Single-ended mode and read ADC Channel 0 
	   command in byte 2 and dummy bytes to get the
	   results from SPI in byte 3 */
	SPI0_DATA = 0X01;		
	SPI0_DATA = 0X80 | (channel << 6);		
	SPI0_DATA = 0X00;	

	/* Wait for Tx complete set - DONE=1 */
	while ((SPI0_CS & 0x10000) == 0u)
	{
	}

	/* Read ADC CH0 results from RX FIFO if not empty */
	while ((SPI0_CS & 0x20000) != 0u)
	{
		result[0] = SPI0_DATA & 0xFF;
		
		/* Most Significant 2 bits of ADC. 
		we are only interested in bit 0 and 1 of the result */
		result[1] = SPI0_DATA & 0x03; 
		result[2] = SPI0_DATA & 0xFF; /* Remaining 8 bits of ADC */
	}

	SPI0_CS &= ~(1<<2); /* TA inactive */
	
	/* Extract 10 bits of AD counts from SPI */
	thermistor_counts = (result[1] << 8) | (result[2]);
}

return(thermistor_counts);
}	

/*========================================================= 
Name: ramp_temperature
Description:
This function ramps the temperature according to the ramp rate
*=========================================================*/
static float ramp_temperature(float temp_ramp, float temp)
{
float temp_diff; /* Change in temperature */

temp_diff = calc_diff(temp_ramp, temp);

if (temp_diff <= RAMP_RATE)
{
	temp_ramp = temp;				
}
else
{
	if (temp_ramp > temp)
	{
		temp_ramp -= RAMP_RATE;
	}
	else
	{
		temp_ramp += RAMP_RATE;
	}						
}

return(temp_ramp);
}

/*========================================================= 
Name: calc_diff
Description:
This function calculates the difference between its two input
parameters.
*=========================================================*/
static float calc_diff(float temp_ramp, float temp)
{
float temp_diff;

if (temp_ramp > temp)
{
	temp_diff = (temp_ramp - temp);
}
else
{
	temp_diff = (temp - temp_ramp);
}
return(temp_diff);
}
