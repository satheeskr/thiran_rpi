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

#define PERIPHERAL_BASE (0x3F000000)
#define GPIO_BASE (PERIPHERAL_BASE + 0x200000)
#define STM_BASE (0x3F003000)
#define INPORT(a) 		*(gpio + a/10) &= ~(unsigned int)(7<<((a%10)*3))
#define OUTPORT(a) 		*(gpio + a/10) |= (unsigned int)(1<<((a%10)*3))
#define SET_PORT(a) 		*(gpio + 7 + a/32) = (unsigned int)(1<<a)
#define CLR_PORT(a) 		*(gpio + 10 + a/32) = (unsigned int)(1<<a)

#define PORT_NUM 23u

#define PULSE_SHORT0_BELL (370u)
#define PULSE_LONG0_BELL (630u)
#define PULSE_SHORT1_BELL (300u)
#define PULSE_LONG1_BELL (700u)

inline DELAY_USECONDS(unsigned int delay)
{
	start_time = *(volatile unsigned int *)(stm + 1);
	current_time = start_time;
	while((current_time - start_time) < delay)
	{
		current_time = *(unsigned int *)(stm + 1);
	}
}


/* This function sends the binary code 0 and 1 to the wireless
   bell. 0 is sent as a 630us low pulse folowed by 370us high 
   pulse. 1 is sent as a 300us low pulse followed by 700us 
   high pulse. Sync pulse is sent between the byte codes. Sync 
   pulse is sent as a 11ms (300us x 39) low pulse.
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
	DELAY_USECONDS(PULSE_SHORT0_BELL);				

	while(bytecode[i] != '\0')
	{		
		/* Send bit 1 */
		if('1' == bytecode[i])
		{
			CLR_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_SHORT1_BELL);
			SET_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_LONG1_BELL);
		}
		else /* Send bit 0 */
		{
			CLR_PORT(PORT_NUM);
			DELAY_USECONDS(PULSE_LONG0_BELL);						
			SET_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_SHORT0_BELL);						
		}
		i++;
	}

	/* Send Sync bits */
	CLR_PORT(PORT_NUM);	
	DELAY_USECONDS((PULSE_SHORT0_BELL * 32));			
}
}

int main(int argc, char *argv[])
{
int fp, fp1;
void * gpio_map;
void * stm_base; 
unsigned char * bytecode;
volatile unsigned int cmd;

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

/* Ding-dong */
send_code_bell("010011010011");
INPORT(PORT_NUM);
}
