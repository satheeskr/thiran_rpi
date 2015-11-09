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
#define EVENT_CLEAR(a) 		*(gpio + 16 + a/32) = (unsigned int)(1<<a)
#define PIN_LEVEL(a) 		(*(gpio + 13 + a/32)>>a)

#define PORT_NUM 23u
#define PORT_IN_NUM 27u

#define PULSE_SHORT_SOCK 350u
#define PULSE_LONG_SOCK (PULSE_SHORT_SOCK * 3u)

#define PULSE_SHORT_BELL 280u
#define PULSE_LONG_BELL (PULSE_SHORT_BELL * 2u)

inline DELAY_USECONDS(unsigned int delay)
{
	start_time = *(volatile unsigned int *)(stm + 1);
	current_time = start_time;
	while((current_time - start_time) < delay)
	{
		current_time = *(unsigned int *)(stm + 1);
	}
}

/* This function sends the binary code 0 and 1 to the remote
   switch. 0 is sent as a 350us high pulse followed by 1050us 
   low pulse. 1 is sent as a 1050us high pulse followed by 350us 
   low pulse. Sync pulse is sent between the byte codes. Sync 
   pulse is sent as 350us high pulse followed by (350us x 31)
   low pulse. This is repeated 10 times so that the receiver 
   catches at least one */
void send_code_sock(unsigned char * bytecode)
{
int i;
int x;
for (x = 0; x < 10; x++)
{
	i = 0;

	while(bytecode[i] != '\0')
	{		
		/* Send bit 1 */
		if('1' == bytecode[i])
		{
			SET_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_LONG_SOCK);
			CLR_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_SHORT_SOCK);
		}
		else /* Send bit 0 */
		{
			SET_PORT(PORT_NUM);
			DELAY_USECONDS(PULSE_SHORT_SOCK);						
			CLR_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_LONG_SOCK);						
		}
		i++;
	}

	/* Send Sync bits */
	SET_PORT(PORT_NUM);	
	DELAY_USECONDS(PULSE_SHORT_SOCK);				
	CLR_PORT(PORT_NUM);
	DELAY_USECONDS((PULSE_SHORT_SOCK * 36));			
}
}

/* This function sends the binary code 0 and 1 to the wireless
   bell. 0 is sent as a 280us low pulse folowed by 560us high 
   pulse. 1 is sent as a 560us low pulse followed by 280us 
   high pulse. Sync pulse is sent between the byte codes. Sync 
   pulse is sent as a 10ms (280us x 36) low pulse.
   This is repeated 10 times so that the receiver 
   catches at least one */
void send_code_bell(unsigned char * bytecode)
{
int i;
int x;
for (x = 0; x < 10; x++)
{
	i = 0;

	/* Send Start bit */
	SET_PORT(PORT_NUM);	
	DELAY_USECONDS(PULSE_SHORT_BELL);				

	while(bytecode[i] != '\0')
	{		
		/* Send bit 1 */
		if('1' == bytecode[i])
		{
			CLR_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_LONG_BELL);
			SET_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_SHORT_BELL);
		}
		else /* Send bit 0 */
		{
			CLR_PORT(PORT_NUM);
			DELAY_USECONDS(PULSE_SHORT_BELL);						
			SET_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_LONG_BELL);						
		}
		i++;
	}

	/* Send Sync bits */
	CLR_PORT(PORT_NUM);	
	DELAY_USECONDS((PULSE_SHORT_BELL * 36));			
}
}

int main(int argc, char *argv[])
{
int fp, fp1;
void * gpio_map;
void * stm_base; 
unsigned char * bytecode_sock;
unsigned char * bytecode_bell;
volatile unsigned int cmd;
static unsigned char toggle = 0u;

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

/* Configure port 23 as output */
INPORT(PORT_NUM);
OUTPORT(PORT_NUM);

/* Configure port 27 as input */
INPORT(PORT_IN_NUM);
FALLING_EDGE(PORT_IN_NUM);

while(1)
{
sleep(1);
if (DETECT_EDGE(PORT_IN_NUM) == 1u)
{
	EVENT_CLEAR(PORT_IN_NUM);
	
	if (0u == toggle)
	{
		/* Switch 2 on */
		bytecode_sock = "000000010101000001010101"; /* 0x015055 */
		toggle = 1u;
	}
	else
	{
		/* Switch 1 off */
		bytecode_sock = "000000010101000001010100"; /* 0x015054 */		
		toggle = 0u;
	}
	
	bytecode_bell = "011100000100"; /* 0x8FB */
	
	/* Transmit code to turn off/on the remote switch */
	send_code_sock(bytecode_sock);
	
	/* Ding-dong */
	send_code_bell(bytecode_bell);
}
}
INPORT(PORT_NUM);
}
