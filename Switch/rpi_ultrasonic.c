#include "stdio.h"
#include "stdlib.h"
#include "sys/mman.h"
#include "sys/types.h" 
#include "sys/stat.h"
#include "fcntl.h"
#include "unistd.h"
#include "math.h"

volatile unsigned int * gpio;
volatile unsigned int * stm;
volatile unsigned int start_time = 0; 
volatile unsigned int current_time = 0; 

#define PERIPHERAL_BASE (0x3F000000)
#define GPIO_BASE (PERIPHERAL_BASE + 0x200000)
#define STM_BASE  (PERIPHERAL_BASE + 0x3000)
#define INPORT(a) 		*(gpio + a/10) &= ~(unsigned int)(7<<((a%10)*3))
#define OUTPORT(a) 		*(gpio + a/10) |= (unsigned int)(1<<((a%10)*3))
#define SET_PORT(a) 		*(gpio + 7 + a/32) = (unsigned int)(1<<a)
#define CLR_PORT(a) 		*(gpio + 10 + a/32) = (unsigned int)(1<<a)
#define DETECT_EDGE(a)		(*(gpio + 16 + a/32)>>a)
#define FALLING_EDGE(a)		*(gpio + 22 + a/32) = (unsigned int)(1<<a)
#define RISING_EDGE(a)		*(gpio + 19 + a/32) = (unsigned int)(1<<a)
#define EVENT_CLEAR(a) 		*(gpio + 16 + a/32) = (unsigned int)(1<<a)
#define PIN_LEVEL(a) 		(*(gpio + 13 + a/32)>>a)

#define PORT_NUM 20u
#define PORT_IN_NUM 21u

#define TRUE 1
#define FALSE 0
#define U32_MAX 4294967295uL

inline DELAY_USECONDS(unsigned int delay)
{
	start_time = *(volatile unsigned int *)(stm + 1);
	current_time = start_time;
	while((current_time - start_time) < delay)
	{
		current_time = *(unsigned int *)(stm + 1);
	}
}

int main(int argc, char *argv[])
{
int fp, fp1;
void * gpio_map;
void * stm_base; 
unsigned char echo_start = FALSE;
unsigned int echo_duration = 0u;
float distance = 0.0f;
float prev_distance = 0.0f;
float dist_diff = 0.0f;
unsigned int curr_time = 0u;
unsigned int prev_time = 0u;

if((fp = open("/dev/mem", O_RDWR|O_SYNC)) < 0)
{
	printf("Cannot open memory map for write \n");
	exit(-1);
}

gpio_map = mmap(NULL, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, fp, GPIO_BASE); 
gpio = (volatile unsigned int *)gpio_map;

if(MAP_FAILED == gpio_map)
{
	printf("Memory map failed\n");
	exit(-1);
}

if((fp1 = open("/dev/mem", O_RDWR|O_SYNC)) < 0)
{
	printf("Cannot open memory map for write \n");
	exit(-1);
}

stm_base = mmap(NULL, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, fp1, STM_BASE);
stm = (volatile unsigned int *)stm_base;

close(fp);
close(fp1);

/* Configure port 23 as output for sending the trigger signal */
INPORT(PORT_NUM);
OUTPORT(PORT_NUM);

/* Configure port 20 as input for receiving the echo signal */
INPORT(PORT_IN_NUM);
RISING_EDGE(PORT_IN_NUM);
FALLING_EDGE(PORT_IN_NUM);

while(TRUE)
{
/* Clear any existing events */
EVENT_CLEAR(PORT_IN_NUM);

/* Send trigger signal 10us high _|-|_ */
SET_PORT(PORT_NUM);
DELAY_USECONDS(10u);
CLR_PORT(PORT_NUM);

/* Wait for echo */
while(TRUE)
{
	/* Check for valid echo signal */
	if (DETECT_EDGE(PORT_IN_NUM) == 1u)
	{
		/* Clear edge */
		EVENT_CLEAR(PORT_IN_NUM);

	        curr_time = *(volatile unsigned int *)(stm + 1);

		/* Enter if echo start has been received */
		if (TRUE == echo_start)
		{
        		/* U32 roll-over check */
        		if (curr_time > prev_time)
        		{
                		echo_duration = curr_time - prev_time;
        		}
        		else
        		{
                		echo_duration = U32_MAX - curr_time + prev_time + 1u;
        		}

			/* distance travelled = velocity * time taken
			   speed of sound in air = 340 m/s 
			   time taken is for to and fro from the obstacle so
			   divide by 2 in microseconds; 340/2 * 100/1000000 cm/s */
			distance = 0.017f * echo_duration;
			
			/* Check if movement is significant; more than 50 mm */
			dist_diff = distance - prev_distance;

			if (fabs(dist_diff) > 0.5f)
			{
				prev_distance = distance;
				printf("Echo duration: %d us, Distance: %f cm\n", echo_duration, distance);
			}

			/* Clear results */
			echo_start = FALSE;
			echo_duration = 0u;
			distance = 0.0f;
			curr_time = 0u;
			prev_time = 0u;

			/* Come out of the loop to start the next measurement */
			break;
		}
		prev_time = curr_time;
		echo_start = TRUE;
	}
}
sleep(1);
}
INPORT(PORT_NUM);
}
