/*=============================================
Name: rpi_receiver_pir.c
Author: Sathees Balya
Description:
This file implements the features required to
receive Off-On Keying (OOK) modulated RF signals 
from the 433MHz receiver. It currently supports 
Friedland PIR sensor, NEXA remote switch and
magnetic door sensor, Door bells from Byron
and 1-by-one. After receiving the codes, it 
turns the lamps and sends a picture to the
configured email address.
==============================================*/
#include "rpi_receiver_pir.h"

const pulse_t protocol_table[MAX_PROTOCOL] = 
{
	/* PIR */
	{P1_ENABLED, 1300 * TL1, 1300 * TH1, 1040 * TL1, 1040 * TH1, 2080 * TL1, 2080 * TH1, 23500,       24500,          0,          0,       129,	1,	129},
	/* BELL */
	{P2_ENABLED,  280 * TL2,  280 * TH2,  280 * TL2,  280 * TH2,  560 * TL2,  560 * TH2,  9980,       10180,          0,          0,        25,	1,	 25},
	/* BYRON BELL */
	{P3_ENABLED,  330 * TL3,  330 * TH3,  330 * TL3,  330 * TH3,  660 * TL3,  660 * TH3, 10692,       13068,          0,          0,        25,	1,	 25},
	/* SMOKE */
	{P4_ENABLED,  180 * TL4,  180 * TH4,  180 * TL4,  180 * TH4,  540 * TL4,  540 * TH4,  7050,        7150,          0,          0,        50,     0,	 48},
	/* NEXA */
	{P5_ENABLED,  265 * TL5,  265 * TH5,  265 * TL5,  265 * TH5, 1325 * TL5, 1325 * TH5, 10070 * TL5, 10070 * TH5, 2650 * TL5, 2650 * TH5, 130,	2,	130},
};

const pir_t pir_table[MAX_NUM_PIR] = 
{
	{PIR1_CODE1, PIR1_CODE2, PIR1_CODE3},
	{PIR2_CODE1, PIR2_CODE2, PIR2_CODE3},
	{PIR3_CODE1, PIR3_CODE2, PIR3_CODE3},
	{PIR4_CODE1, PIR4_CODE2, PIR4_CODE3},
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
unsigned char code_valid = 0;
unsigned char protocol;
unsigned short i = 0u;
unsigned short bits = 0u;
unsigned short code[4] = {0};
unsigned short prev_code[MAX_NUM_PIR][4] = {0};

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
	
	/* U32 roll-over check */
	if (curr_time > prev_time)
	{
		pulse_duration = curr_time - prev_time;
	}
	else
	{
		pulse_duration = U32_MAX - curr_time + prev_time + 1u;
	}

	/* Detect long inter-frame gap and the start bit to identify the real message */
	if ((FALSE == start_detected) && (FALSE == inter_frame_gap))
	{
		for (protocol = 0; protocol < MAX_PROTOCOL; protocol++)
		{
	    		if ((TRUE == protocol_table[protocol].enabled) &&
				(pulse_duration >  protocol_table[protocol].if_gap_min) &&
				(pulse_duration < protocol_table[protocol].if_gap_max))
			{
				inter_frame_gap = TRUE;
				break;
			}
		}
	}
	else if (TRUE == inter_frame_gap)
	{
		inter_frame_gap = FALSE;

 		if ((pulse_duration > protocol_table[protocol].start_min) &&
			(pulse_duration < protocol_table[protocol].start_max))
		{
			start_detected = TRUE;
			pls_dur[pulse_count++] = pulse_duration;
			continue;
		}
	}
	else
	{
		/* Ignore the pulse */
		inter_frame_gap = FALSE;
	}

	if (start_detected == TRUE)
        {
		pls_dur[pulse_count++] = pulse_duration;

		if (BELL == protocol)
		{
			if ((pls_dur[1u] > protocol_table[NEXA].preamble_min) && 
				(pls_dur[1u] < protocol_table[NEXA].preamble_max))
			{
				protocol = NEXA;
			}
		}

		if(pulse_count == protocol_table[protocol].max_bits)
		{
		start_detected = FALSE;
#ifdef DEBUG
		printf("protocol %d\n", protocol);
		printf("pulse start duration %d\n", pls_dur[0]);
#endif
		if (NEXA > protocol)
		{
			pulse_count = 0;
			for( i = protocol_table[protocol].start_bit; i < protocol_table[protocol].end_bit; i+=2)
			{
				code[i / 32] <<= 1;
#ifdef DEBUG
				printf("pulse duration %d %d\n", pls_dur[i], pls_dur[i+1]);
#endif
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
#ifndef DEBUG		
					/* Incorrect pulse, reset code */
					code[0] = 0;
					code[1] = 0;
					code[2] = 0;
					code[3] = 0;
					
					for (bits = 0; bits < 256; bits++)
					{
						pls_dur[bits] = 0;
					}
					break;
#endif
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
				unsigned char code_matched = 0u;
				unsigned char num_pir = 0u;

				for (num_pir = 0u; num_pir < MAX_NUM_PIR; num_pir++)
				{
					if ((code[1] == pir_table[num_pir].code1) && 
						(code[2] == pir_table[num_pir].code2) &&
						((0xFF00 & code[3]) == pir_table[num_pir].code3))
					{
						code_matched = num_pir + 1u;
#ifdef DEBUG		
						printf("PIR %d detected\n", code_matched);
#endif
						break;
					}

				}

				if (0 != code_matched)
				{
					code_valid = 1;

					if ((code[0] & 0xFF) != (prev_code[code_matched - 1][0] & 0xFF))
					{
						system("/home/pi/pir_response_start.sh");
						sprintf(cmd,"%s %0x %0x %0x %0x %d &", "/home/pi/webcam.sh", code[0], code[1], code[2], code[3], code_matched);
						system(cmd);
						system("/home/pi/pir_response_end.sh &");
					}

					prev_code[code_matched - 1][0] = code[0];
					prev_code[code_matched - 1][1] = code[1];
					prev_code[code_matched - 1][2] = code[2];
					prev_code[code_matched - 1][3] = code[3];

					/* Reset code */
					code_matched = 0;
				}
			}
			else if ((BELL == protocol) && (BELL_CODE == code[0]))
			{
				code_valid = 1;

				if (toggle)
				{
					toggle = 0;
					system("/home/pi/pyradio_on.sh &");
				}
				else
				{
					toggle = 1;
					system("/home/pi/pyradio_off.sh &");
				}
			}
			else if ((BYRON == protocol) && (BYRON_CODE == code[0]))
			{
				code_valid = 1u;

				system("/home/pi/pir_response_start.sh");
				sprintf(cmd,"%s %0x %0x %0x %0x %d &", "/home/pi/webcam.sh", code[0], code[1], code[2], code[3], 40);
				system(cmd);
				system("/home/pi/pir_response_end.sh &");
			}
			else if ((SMOKE == protocol) && (SMOKE_CODE1 == code[0u]) && (SMOKE_CODE2 == code[1u]))
			{
				code_valid = 1u;
				sprintf(cmd,"%s %0x %0x %0x %0x %d &", "/home/pi/webcam.sh", code[0], code[1], code[2], code[3], 50);
				system(cmd);
			}
			else
			{
				/* Do nothing */
			}
		}
		else /* protocol == NEXA */
		{ 
		if ((pls_dur[1u] > protocol_table[protocol].preamble_min) && 
			(pls_dur[1u] < protocol_table[protocol].preamble_max))
		{
			pulse_count = 0u;
#ifdef DEBUG
			printf("pulse preamble duration %d\n", pls_dur[1u]);
#endif

			for( i = protocol_table[protocol].start_bit; i < protocol_table[protocol].end_bit; i+=4)
			{
				code[i / 64u] <<= 1u;
#ifdef DEBUG
				printf("pulse duration %d %d %d %d\n", pls_dur[i], pls_dur[i+1], pls_dur[i+2], pls_dur[i+3]);
#endif
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
#ifndef DEBUG
					/* Incorrect pulse, reset code */
					code[0] = 0;
					code[1] = 0;
					code[2] = 0;
					code[3] = 0;

					for (bits = 0; bits < 256; bits++)
					{
						pls_dur[bits] = 0;
					}
					break;
#endif
				}
			}
			if ((0x2EC5 == code[0]) && (0xBABA == code[1]))
			{
				sprintf(cmd,"%s %0x %0x %0x %0x %d &", "/home/pi/webcam.sh", code[0], code[1], code[2], code[3], 99);
				system(cmd);
				code_valid = 1;
			}
			else if ((0x2EC5 == code[0]) && (0xBB90 == code[1]))
			{
				code_valid = 1;
			} 
			else if ((0x2EC5 == code[0]) && (0xBB80 == code[1]))
			{
				code_valid = 1;
			} 
			else if ((0x2EC5 == code[0]) && (0xBB91 == code[1]))
			{
				code_valid = 1;
			} 
			else if ((0x2EC5 == code[0]) && (0xBB81 == code[1]))
			{
				code_valid = 1;
			} 
			else if ((0x2EC5 == code[0]) && (0xBB92 == code[1]))
			{
				code_valid = 1;
			}
			else if ((0x2EC5 == code[0]) && (0xBB82 == code[1]))
			{
				code_valid = 1;
			}
			else if ((0x2EC5 == code[0]) && (0xBBA0 == code[1]))
			{
				code_valid = 1;
			}
			else 
			{
				/* do nothing */
			}

			if ((NEXA_MS1_CODE1 == code[0]) || ((NEXA_MS1_CODE2 == code[1]) || (NEXA_MS1_CODE3 == code[1])))
			{
				code_valid = 1;
				system("/home/pi/pir_response_start.sh");
				sprintf(cmd,"%s %0x %0x %0x %0x %d &", "/home/pi/webcam.sh", code[0], code[1], code[2], code[3], 30);
				system(cmd);
				system("/home/pi/pir_response_end.sh &");

			}
			else
			{
				/* Ignore the code */
			}

		}
		}
#ifdef DEBUG		
		printf("code received is %0x %0x %0x %0x\n", code[0u], code [1u], code[2u], code[3u]);
#endif
		for (bits = 0u; bits < 256u; bits++)
		{
			pls_dur[bits] = 0u;
		}

		/* Reset code */
		code[0u] = 0u;
		code[1u] = 0u;
		code[2u] = 0u;
		code[3u] = 0u;
		
		/* Sleep only after receiving valid code */
		if (1u == code_valid)
		{
			if (SMOKE == protocol)
			{
				sleep(SLEEP_TIME2);
			}
			else
			{
				sleep(SLEEP_TIME1);
			}
		}
		
		/* Reset code valid flag */
		code_valid = 0u;
		}
        }
}
prev_time = curr_time;
}
}
