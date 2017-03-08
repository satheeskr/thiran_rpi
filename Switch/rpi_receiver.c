#include "stdio.h"
#include "stdlib.h"
#include "sys/mman.h"
#include "sys/types.h" 
#include "sys/stat.h"
#include "fcntl.h"
#include "unistd.h"

volatile unsigned int * gpio;
volatile unsigned int * stm;
volatile unsigned int start_time = 0; 
volatile unsigned int current_time = 0; 

#define PERIPHERAL_BASE (0x20000000)
#define GPIO_BASE (PERIPHERAL_BASE + 0x200000)
#define STM_BASE (0x20003000)
#define INPORT(a) 		*(gpio + a/10) &= ~(unsigned int)(7<<((a%10)*3))
#define OUTPORT(a) 		*(gpio + a/10) |= (unsigned int)(1<<((a%10)*3))
#define SET_PORT(a) 		*(gpio + 7 + a/32) = (unsigned int)(1<<a)
#define CLR_PORT(a) 		*(gpio + 10 + a/32) = (unsigned int)(1<<a)
#define DETECT_EDGE(a)		(*(gpio + 16 + a/32)>>a)
#define FALLING_EDGE(a)		*(gpio + 22 + a/32) = (unsigned int)(1<<a)
#define RISING_EDGE(a)		*(gpio + 19 + a/32) = (unsigned int)(1<<a)
#define EVENT_CLEAR(a) 		*(gpio + 16 + a/32) = (unsigned int)(1<<a)
#define PIN_LEVEL(a) 		(*(gpio + 13 + a/32)>>a)

#define PORT_NUM 23u
#define PULSE_SHORT_BELL 280u
#define PULSE_LONG_BELL (PULSE_SHORT_BELL * 2u)
#define PULSE_INTERFRAME_GAP_MIN 9900
#define PULSE_INTERFRAME_GAP_MAX 9920
#define TOLERANCE 20 /* percent */
#define PULSE_SHORT_BELL_MIN (PULSE_SHORT_BELL - (PULSE_SHORT_BELL * TOLERANCE)/100)
#define PULSE_SHORT_BELL_MAX (PULSE_SHORT_BELL + (PULSE_SHORT_BELL * TOLERANCE)/100)
#define PULSE_LONG_BELL_MIN (PULSE_LONG_BELL - (PULSE_LONG_BELL * TOLERANCE)/100)
#define PULSE_LONG_BELL_MAX (PULSE_LONG_BELL + (PULSE_LONG_BELL * TOLERANCE)/100)
#define MAX_PULSE_COUNT 30
inline DELAY_USECONDS(unsigned int delay)
{
	start_time = *(volatile unsigned int *)(stm + 1);
	current_time = start_time;
	while((current_time - start_time) < delay)
	{
		current_time = *(unsigned int *)(stm + 1);
	}
}

/* This function receives the binary code 0 and 1 from the wireless
   bell switch. 0 is sent as a 280us low pulse folowed by 560us high 
   pulse. 1 is sent as a 560us low pulse followed by 280us 
   high pulse. Sync pulse is sent between the byte codes. Sync 
   pulse is sent as a 10ms (280us x 36) low pulse.
   This is repeated 10 times so that the receiver 
   catches at least one */
int main(int argc, char *argv[])
{
int fp, fp1;
void * gpio_map;
void * stm_base; 
unsigned int pulse_duration;
unsigned int curr_time = 0;
unsigned int prev_time = 0;
unsigned char counter = 0;
unsigned char toggle = 1;
unsigned char pulse_count = 0;
unsigned char start_detected = 0;
unsigned int pls_dur[MAX_PULSE_COUNT];

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

/* Configure port 23 as input */
INPORT(PORT_NUM);
FALLING_EDGE(PORT_NUM);
RISING_EDGE(PORT_NUM);

while(1)
{
if (DETECT_EDGE(PORT_NUM) == 1u)
{
	EVENT_CLEAR(PORT_NUM);
	curr_time = *(volatile unsigned int *)(stm + 1);
	pulse_duration = curr_time - prev_time;
    	if ((pulse_duration > PULSE_INTERFRAME_GAP_MIN) && 
		(pulse_duration < PULSE_INTERFRAME_GAP_MAX) &&
		(0 == start_detected))
	{
		counter++;
 		if (counter == 3)
		{
			start_detected = 1;
			counter = 0;
		}
	}
	
	if (start_detected == 1)
        {
		pls_dur[pulse_count++] = pulse_duration;

		if (pulse_count == MAX_PULSE_COUNT)
		{
			pulse_count = 0;
			start_detected = 0;
			unsigned char i;
			unsigned short code = 0;

			for( i = 2; i < MAX_PULSE_COUNT; i+=2)
			{
				code <<= 1;

				if ((pls_dur[i] > PULSE_SHORT_BELL_MIN) && 
					(pls_dur[i] < PULSE_SHORT_BELL_MAX) && 
					(pls_dur[i+1] > PULSE_LONG_BELL_MIN) && 
					(pls_dur[i+1] < PULSE_LONG_BELL_MAX))
				{
					/* zero detected */
				}
				else if ((pls_dur[i+1] > PULSE_SHORT_BELL_MIN) && 
						(pls_dur[i+1] < PULSE_SHORT_BELL_MAX) && 
						(pls_dur[i] > PULSE_LONG_BELL_MIN) && 
						(pls_dur[i] < PULSE_LONG_BELL_MAX))		
				{
					code |= 1;
				}
				else
				{
					if (pls_dur[i] > PULSE_INTERFRAME_GAP_MIN)
					{
						code >>= 1;
						break;
					}
				}
			}

			printf("code received is %0x\n", code);
		
			if (0x704 == code)
			{
	    			if (toggle)
	    			{
	    				//system("sudo -u pi ssh -lpi 192.168.1.18 sudo /var/www/rf/nexa 5");
	    				system("sudo -u pi ssh -lpi 192.168.1.18 sudo -u pi /home/pi/fmradio.sh &");
					toggle = 0;
	    			}
	    			else
	    			{
	    				//system("sudo -u pi ssh -lpi 192.168.1.18 sudo /var/www/rf/nexa 6");
					system("sudo -u pi ssh -lpi 192.168.1.18 sudo pkill mplayer &");
					toggle = 1;
	    			}
			}

			sleep(2);
		}
        }	
}
prev_time = curr_time;
}
}
