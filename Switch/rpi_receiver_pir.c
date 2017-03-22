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

#define TOLERANCE_PIR 10u /* percent */
#define PULSE_SHORT_PIR 1040u
#define PULSE_LONG_PIR (PULSE_SHORT_PIR * 2u)
#define PULSE_IF_GAP_PIR_MIN 23900u
#define PULSE_IF_GAP_PIR_MAX 24000u
#define PULSE_SHORT_PIR_MIN (PULSE_SHORT_PIR - (PULSE_SHORT_PIR * TOLERANCE_PIR)/100)
#define PULSE_SHORT_PIR_MAX (PULSE_SHORT_PIR + (PULSE_SHORT_PIR * TOLERANCE_PIR)/100)
#define PULSE_LONG_PIR_MIN (PULSE_LONG_PIR - (PULSE_LONG_PIR * TOLERANCE_PIR)/100)
#define PULSE_LONG_PIR_MAX (PULSE_LONG_PIR + (PULSE_LONG_PIR * TOLERANCE_PIR)/100)
#define PULSE_START_PIR	1300u
#define PULSE_START_PIR_MIN (PULSE_START_PIR - (PULSE_START_PIR * TOLERANCE_PIR)/100)
#define PULSE_START_PIR_MAX (PULSE_START_PIR + (PULSE_START_PIR * TOLERANCE_PIR)/100)
#define MAX_PULSE_COUNT_PIR 129u

#define TOLERANCE_BELL 20u /* percent */
#define PULSE_SHORT_BELL 280u
#define PULSE_LONG_BELL (PULSE_SHORT_BELL * 2u)
#define PULSE_IF_GAP_BELL (PULSE_SHORT_BELL * 36)
#define PULSE_IF_GAP_BELL_MIN (PULSE_IF_GAP_BELL - (PULSE_IF_GAP_BELL * 1)/100)
#define PULSE_IF_GAP_BELL_MAX (PULSE_IF_GAP_BELL + (PULSE_IF_GAP_BELL * 1)/100)
#define PULSE_SHORT_BELL_MIN (PULSE_SHORT_BELL - (PULSE_SHORT_BELL * TOLERANCE_BELL)/100)
#define PULSE_SHORT_BELL_MAX (PULSE_SHORT_BELL + (PULSE_SHORT_BELL * TOLERANCE_BELL)/100)
#define PULSE_LONG_BELL_MIN (PULSE_LONG_BELL - (PULSE_LONG_BELL * TOLERANCE_BELL)/100)
#define PULSE_LONG_BELL_MAX (PULSE_LONG_BELL + (PULSE_LONG_BELL * TOLERANCE_BELL)/100)
#define PULSE_START_BELL 280u
#define PULSE_START_BELL_MIN (PULSE_START_BELL - (PULSE_START_BELL * TOLERANCE_BELL)/100)
#define PULSE_START_BELL_MAX (PULSE_START_BELL + (PULSE_START_BELL * TOLERANCE_BELL)/100)
#define MAX_PULSE_COUNT_BELL 25u

#define TOLERANCE_BYRON 20u /* percent */
#define PULSE_SHORT_BYRON 330u
#define PULSE_LONG_BYRON (PULSE_SHORT_BYRON * 2u)
#define PULSE_IF_GAP_BYRON (PULSE_SHORT_BYRON * 36u)
#define PULSE_IF_GAP_BYRON_MIN (PULSE_IF_GAP_BYRON - (PULSE_IF_GAP_BYRON * 10)/100)
#define PULSE_IF_GAP_BYRON_MAX (PULSE_IF_GAP_BYRON + (PULSE_IF_GAP_BYRON * 10)/100)
#define PULSE_SHORT_BYRON_MIN (PULSE_SHORT_BYRON - (PULSE_SHORT_BYRON * TOLERANCE_BELL)/100)
#define PULSE_SHORT_BYRON_MAX (PULSE_SHORT_BYRON + (PULSE_SHORT_BYRON * TOLERANCE_BELL)/100)
#define PULSE_LONG_BYRON_MIN (PULSE_LONG_BYRON - (PULSE_LONG_BYRON * TOLERANCE_BELL)/100)
#define PULSE_LONG_BYRON_MAX (PULSE_LONG_BYRON + (PULSE_LONG_BYRON * TOLERANCE_BELL)/100)
#define PULSE_START_BYRON 330u
#define PULSE_START_BYRON_MIN (PULSE_START_BYRON - (PULSE_START_BYRON * TOLERANCE_BYRON)/100)
#define PULSE_START_BYRON_MAX (PULSE_START_BYRON + (PULSE_START_BYRON * TOLERANCE_BYRON)/100)
#define MAX_PULSE_COUNT_BYRON 25u

#define TOLERANCE_NEXA 40u /* percent */
#define PULSE_SHORT_NEXA 265u
#define PULSE_LONG_NEXA (PULSE_SHORT_NEXA * 5u)
#define PULSE_IF_GAP_NEXA (PULSE_SHORT_NEXA * 37)
#define PULSE_IF_GAP_NEXA_MIN (PULSE_IF_GAP_NEXA - (PULSE_IF_GAP_NEXA * 10)/100)
#define PULSE_IF_GAP_NEXA_MAX (PULSE_IF_GAP_NEXA + (PULSE_IF_GAP_NEXA * 10)/100)
#define PULSE_SHORT_NEXA_MIN (PULSE_SHORT_NEXA - (PULSE_SHORT_NEXA * TOLERANCE_NEXA)/100)
#define PULSE_SHORT_NEXA_MAX (PULSE_SHORT_NEXA + (PULSE_SHORT_NEXA * TOLERANCE_NEXA)/100)
#define PULSE_LONG_NEXA_MIN (PULSE_LONG_NEXA - (PULSE_LONG_NEXA * TOLERANCE_NEXA)/100)
#define PULSE_LONG_NEXA_MAX (PULSE_LONG_NEXA + (PULSE_LONG_NEXA * TOLERANCE_NEXA)/100)
#define PULSE_START_NEXA 265u
#define PULSE_START_NEXA_MIN (PULSE_START_NEXA - (PULSE_START_NEXA * 10)/100)
#define PULSE_START_NEXA_MAX (PULSE_START_NEXA + (PULSE_START_NEXA * 10)/100)
#define MAX_PULSE_COUNT_NEXA 130

#define MAX_PROTOCOL	4u

#define PIR 	0
#define BELL 	1
#define BYRON 	2
#define NEXA 	3


typedef struct
{
	unsigned int start_min;
	unsigned int start_max;
	unsigned int short_min;
	unsigned int short_max;
	unsigned int long_min;
	unsigned int long_max;
	unsigned int if_gap_min;
	unsigned int if_gap_max;
	unsigned int max_bits;
}pulse_t;

const pulse_t protocol_table[MAX_PROTOCOL] = 
{
	{ PULSE_START_PIR_MIN, PULSE_START_PIR_MAX, PULSE_SHORT_PIR_MIN, PULSE_SHORT_PIR_MAX, PULSE_LONG_PIR_MIN, PULSE_LONG_PIR_MAX, PULSE_IF_GAP_PIR_MIN, PULSE_IF_GAP_PIR_MAX, MAX_PULSE_COUNT_PIR},
	{ PULSE_START_BELL_MIN, PULSE_START_BELL_MAX, PULSE_SHORT_BELL_MIN, PULSE_SHORT_BELL_MAX, PULSE_LONG_BELL_MIN, PULSE_LONG_BELL_MAX, PULSE_IF_GAP_BELL_MIN, PULSE_IF_GAP_BELL_MAX, MAX_PULSE_COUNT_BELL},
	{ PULSE_START_BYRON_MIN, PULSE_START_BYRON_MAX, PULSE_SHORT_BYRON_MIN, PULSE_SHORT_BYRON_MAX, PULSE_LONG_BYRON_MIN, PULSE_LONG_BYRON_MAX, PULSE_IF_GAP_BYRON_MIN, PULSE_IF_GAP_BYRON_MAX, MAX_PULSE_COUNT_BYRON},
	{ PULSE_START_NEXA_MIN, PULSE_START_NEXA_MAX, PULSE_SHORT_NEXA_MIN, PULSE_SHORT_NEXA_MAX, PULSE_LONG_NEXA_MIN, PULSE_LONG_NEXA_MAX, PULSE_IF_GAP_NEXA_MIN, PULSE_IF_GAP_NEXA_MAX, MAX_PULSE_COUNT_NEXA},
};

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
unsigned int pulse_duration;
unsigned int curr_time = 0;
unsigned int prev_time = 0;
unsigned char inter_frame_gap = 0;
unsigned char toggle = 1;
unsigned char pulse_count = 0;
unsigned char start_detected = 0;
unsigned int pls_dur[255];
unsigned char cmd[100];

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
	
	unsigned char protocol;
	unsigned short i;
	unsigned short code[4] = {0};
	unsigned short prev_code[3][4] = {0};

	/* Detect long inter-frame gap and the start bit to identify the real message */
	if ((0 == start_detected) && (0 == inter_frame_gap))
	{
		for (protocol = 0; protocol < MAX_PROTOCOL; protocol++)
		{
	    		if ((pulse_duration >  protocol_table[protocol].if_gap_min) &&
				(pulse_duration < protocol_table[protocol].if_gap_max))
			{
				inter_frame_gap = 1;
				break;
			}
		}
	}
	else if (1 == inter_frame_gap)
	{
		inter_frame_gap = 0;

 		if ((pulse_duration > protocol_table[protocol].start_min) &&
			(pulse_duration < protocol_table[protocol].start_max))
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

		if(pulse_count == protocol_table[protocol].max_bits)
		{
		start_detected = 0;

		if (NEXA > protocol)
		{
			pulse_count = 0;
			for( i = 1; i < protocol_table[protocol].max_bits; i+=2)
			{
				code[i / 32] <<= 1;
				//printf("pulse duration %d %d\n", pls_dur[i], pls_dur[i+1]);
				if ((pls_dur[i] > protocol_table[protocol].short_min) && 
					(pls_dur[i] < protocol_table[protocol].short_max) && 
					(pls_dur[i+1] > protocol_table[protocol].long_min) && 
					(pls_dur[i+1] < protocol_table[protocol].long_max))
				{
					/* zero detected */
				}
				else if ((pls_dur[i+1] > protocol_table[protocol].short_min) && 
					(pls_dur[i+1] < protocol_table[protocol].short_max) && 
					(pls_dur[i] > protocol_table[protocol].long_min) && 
					(pls_dur[i] < protocol_table[protocol].long_max))
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
					
					for (i = 0; i < 255; i++)
					{
						pls_dur[i] = 0;
					}
					break;
				}
			}

			/* This function receives the binary code 0 and 1 from the wireless
			   pir/bell switch. 0 is sent as a 1045ms low pulse folowed by 2090ms high 
			   pulse. 1 is sent as a 2090ms low pulse followed by 1045ms 
			   high pulse. Sync pulse is sent between the byte codes. Sync 
			   pulse is sent as a 24ms low pulse. Start pulse is 1300ms high pulse.
			   This is repeated 2 times so that the receiver catches at least one. */

			if (PIR == protocol)
			{
				unsigned char code_matched = 0;
				if ((code[1] == 0xF8DA) && (code[2] == 0x1831) && ((code[3] & 0xFF00) == 0x2200))
				{
//					printf("PIR 1 detected\n");
					code_matched = 1;
				}
				else if ((code[1] == 0x6416) && (code[2] == 0x042C) && ((code[3] & 0xFF00) == 0x2200))
				{
//					printf("PIR 2 detected\n");
					code_matched = 2;
				}
				else
				{
					code_matched = 0;
				}

				if (0 != code_matched)
				{
					if ((code[0] & 0xFF) != (prev_code[code_matched - 1][0] & 0xFF))
					{
						sprintf(cmd,"%s %0x %0x %0x %0x %d", "/home/pi/webcam.sh &", code[0], code[1], code[2], code[3], code_matched);

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
					code_matched = 0;
				}
			}

			if ((BELL == protocol) && (0x704 == code[0]))
			{
				if (toggle)
				{
					toggle = 0;
					system("sudo -u pi ssh -lpi 192.168.1.18 /home/pi/fmradio.sh &");
				}
				else
				{
					toggle = 1;
					system("sudo -u pi ssh -lpi 192.168.1.18 sudo pkill mplayer &");
				}
			}

			if ((BYRON == protocol) && (0xB2C == code[0]))
			{
				system("sudo -u pi ssh -lpi 192.168.1.18 sudo /var/rf/www/nexa 3");
				sprintf(cmd,"%s %0x %0x %0x %0x %d", "/home/pi/webcam.sh", code[0], code[1], code[2], code[3], 4);
				system(cmd);
				system("sudo -u pi ssh -lpi 192.168.1.18 sudo /var/rf/www/nexa 4");
			}
		}
		else
		{ 
		if ((pls_dur[1] > 2300) && (pls_dur[1] < 2900))
		{
			pulse_count = 0;
//			printf("pulse duration %d %d\n", pls_dur[0], pls_dur[1]);
			for( i = 2; i < protocol_table[protocol].max_bits; i+=4)
			{
				code[i / 64] <<= 1;
//				printf("pulse duration %d %d %d %d\n", pls_dur[i], pls_dur[i+1], pls_dur[i+2], pls_dur[i+3]);
				if ((pls_dur[i] > protocol_table[protocol].short_min) && 
					(pls_dur[i] < protocol_table[protocol].short_max) && 
					(pls_dur[i+1] > protocol_table[protocol].short_min) && 
					(pls_dur[i+1] < protocol_table[protocol].short_max) &&
					(pls_dur[i+2] > protocol_table[protocol].short_min) && 
					(pls_dur[i+2] < protocol_table[protocol].short_max) &&
					(pls_dur[i+3] > protocol_table[protocol].long_min) && 
					(pls_dur[i+3] < protocol_table[protocol].long_max))
				{
					/* zero detected */
				}
				else if ((pls_dur[i] > protocol_table[protocol].short_min) && 
					(pls_dur[i] < protocol_table[protocol].short_max) && 
					(pls_dur[i+1] > protocol_table[protocol].long_min) && 
					(pls_dur[i+1] < protocol_table[protocol].long_max) &&
					(pls_dur[i+2] > protocol_table[protocol].short_min) && 
					(pls_dur[i+2] < protocol_table[protocol].short_max) &&
					(pls_dur[i+3] > protocol_table[protocol].short_min) && 
					(pls_dur[i+3] < protocol_table[protocol].short_max))
				{
					code[i / 64] |= 1;
				}
				else
				{
					/* Incorrect pulse, reset code */
					code[0] = 0;
					code[1] = 0;
					code[2] = 0;
					code[3] = 0;

					for (i = 0; i < 255; i++)
					{
						pls_dur[i] = 0;
					}
					break;
				}
			}
			
			if ((0x2EC5 == code[0]) && (0xBB91 == code[1]))
			{
				system("sudo -u pi ssh -lpi 192.168.1.18 sudo /var/www/rf/byron");
			} 
			else if ((0x2EC5 == code[0]) && (0xBB81 == code[1]))
			{
				system("sudo -u pi ssh -lpi 192.168.1.18 sudo /var/www/rf/byron");
			} 
			else if ((0x2EC5 == code[0]) && (0xBB90 == code[1]))
			{
				system("sudo -u pi ssh -lpi 192.168.1.18 sudo /var/www/rf/byron");
			} 
			else if ((0x2EC5 == code[0]) && (0xBB80 == code[1]))
			{
				system("sudo -u pi ssh -lpi 192.168.1.18 sudo /var/www/rf/byron");
			} 
			else if ((0x2EC5 == code[0]) && (0xBB92 == code[1]))
			{
				system("sudo -u pi ssh -lpi 192.168.1.18 sudo /var/www/rf/byron");
			}
			else if ((0x2EC5 == code[0]) && (0xBB82 == code[1]))
			{
				system("sudo -u pi ssh -lpi 192.168.1.18 sudo /var/www/rf/byron");
			}
			else if ((0x2EC5 == code[0]) && (0xBBA0 == code[1]))
			{
				system("sudo -u pi ssh -lpi 192.168.1.18 sudo /var/www/rf/byron");
			}
			else if ((0x5807 == code[0]) && ((0xF689 == code[1]) || (0xF699 == code[1])))
			{
				system("sudo -u pi ssh -lpi 192.168.1.18 sudo /var/rf/www/nexa 3");
				sprintf(cmd,"%s %0x %0x %0x %0x %d", "/home/pi/webcam.sh", code[0], code[1], code[2], code[3], 3);
				system(cmd);
				system("sudo -u pi ssh -lpi 192.168.1.18 sudo /var/rf/www/nexa 4");
			}
			else
			{
				/* Ignore the code */
			}

		}
		}

		printf("code received is %0x %0x %0x %0x\n", code[0], code [1], code[2], code[3]);

		for (i = 0; i < 255; i++)
		{
			pls_dur[i] = 0;
		}

		/* Reset code */
		code[0] = 0;
		code[1] = 0;
		code[2] = 0;
		code[3] = 0;
		sleep(2);
		}
        }
}
prev_time = curr_time;
}
}
