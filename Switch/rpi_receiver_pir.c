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
#define PULSE_SHORT_BELL 1040u
#define PULSE_LONG_BELL (PULSE_SHORT_BELL * 2u)
#define PULSE_INTERFRAME_GAP_MIN 23900
#define PULSE_INTERFRAME_GAP_MAX 24000
#define TOLERANCE 10 /* percent */
#define PULSE_SHORT_BELL_MIN (PULSE_SHORT_BELL - (PULSE_SHORT_BELL * TOLERANCE)/100)
#define PULSE_SHORT_BELL_MAX (PULSE_SHORT_BELL + (PULSE_SHORT_BELL * TOLERANCE)/100)
#define PULSE_LONG_BELL_MIN (PULSE_LONG_BELL - (PULSE_LONG_BELL * TOLERANCE)/100)
#define PULSE_LONG_BELL_MAX (PULSE_LONG_BELL + (PULSE_LONG_BELL * TOLERANCE)/100)
#define PULSE_START_MIN 1275
#define PULSE_START_MAX 1375

#define MAX_PULSE_COUNT 129

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
   bell switch. 0 is sent as a 1045ms low pulse folowed by 2090ms high 
   pulse. 1 is sent as a 2090ms low pulse followed by 1045ms 
   high pulse. Sync pulse is sent between the byte codes. Sync 
   pulse is sent as a 24ms low pulse. Start pulse is 1300ms high pulse.
   This is repeated 2 times so that the receiver catches at least one. */
int main(int argc, char *argv[])
{
int fp, fp1;
void * gpio_map;
void * stm_base; 
unsigned int pulse_duration;
unsigned int curr_time = 0;
unsigned int prev_time = 0;
unsigned char inter_frame_gap = 0;
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
OUTPORT(7u);
while(1)
{
if (DETECT_EDGE(PORT_NUM) == 1u)
{
	EVENT_CLEAR(PORT_NUM);
	curr_time = *(volatile unsigned int *)(stm + 1);
	pulse_duration = curr_time - prev_time;
    	if ((pulse_duration > PULSE_INTERFRAME_GAP_MIN) && 
		(pulse_duration < PULSE_INTERFRAME_GAP_MAX) &&
		(0 == start_detected) && (0 == inter_frame_gap))
	{
		inter_frame_gap = 1;
	}
	else if (1 == inter_frame_gap)
	{
		inter_frame_gap = 0;

 		if ((pulse_duration > PULSE_START_MIN) &&
			(pulse_duration < PULSE_START_MAX))
		{
			start_detected = 1;
		}
	}
	else
	{
		/* Ignore the pulse */
		inter_frame_gap = 0;
	}

	if (start_detected == 1)
        {
		pls_dur[pulse_count++] = pulse_duration;

		if (pulse_count == MAX_PULSE_COUNT)
		{
			unsigned char i;
			unsigned short code[4] = {0};
			unsigned short prev_code[2][4] = {0};

			pulse_count = 0;
			start_detected = 0;
			for( i = 1; i < MAX_PULSE_COUNT; i+=2)
			{
				code[i / 32] <<= 1;

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
					code[i / 32] |= 1;
				}
				else
				{
					/* Incorrect pulse, reset code */
					code[0] = 0;
					code[1] = 0;
					code[2] = 0;
					code[3] = 0;
					break;
				}
			}
							
			//printf("code received is %0x %0x %0x %0x\n", code[0], code [1], code[2], code[3]);
		
			unsigned char code_matched = 0;
			if ((code[1] == 0xF8DA) && (code[2] == 0x1831) && ((code[3] & 0xFF00) == 0x2200))
			{
				//printf("PIR 1 detected\n");
				code_matched = 1;
			}
			else if ((code[1] == 0x6416) && (code[2] == 0x042C) && ((code[3] & 0xFF00) == 0x2200))
			{
				//printf("PIR 2 detected\n");
				code_matched = 2;
			}
			else
			{
				code_matched = 0;
			}
			
			if (0 != code_matched)
			{
				unsigned char cmd[100];

				if ((code[0] & 0xFF) != (prev_code[code_matched - 1][0] & 0xFF))
				{
					sprintf(cmd,"%s %0x %0x %0x %0x %d", "/home/pi/webcam.sh", code[0], code[1], code[2], code[3], code_matched);

					if (1 == code_matched)
					{
						system("sudo -u pi ssh -lpi 192.168.1.18 /home/pi/pir_response_zone1.sh &");
					}

					if (2 == code_matched)
					{
						system("sudo -u pi ssh -lpi 192.168.1.18 /home/pi/pir_response_zone2.sh &");
					}
					system(cmd);
				}

				prev_code[code_matched - 1][0] = code[0];
				prev_code[code_matched - 1][1] = code[1];
				prev_code[code_matched - 1][2] = code[2];
				prev_code[code_matched - 1][3] = code[3];

				/* Reset code */
				code[0] = 0;
				code[1] = 0;
				code[2] = 0;
				code[3] = 0;
				code_matched = 0;

				sleep(2);
			}
		}
        }	
}
prev_time = curr_time;
}
}
