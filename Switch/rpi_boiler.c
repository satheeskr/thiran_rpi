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

#define PORT_NUM 23u

#define PULSE_SHORT_BOILER 319u
#define PULSE_LONG_BOILER (944u)

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
   boiler receiver. 0 is sent as a 319us low pulse folowed by 319us high 
   pulse. 1 is sent as a 944us low pulse followed by 319us 
   high pulse. Each frame starts with a high signal for 4.5ms. There are 
   two packets per frame. Each packet is 33 bits long and are separated 
   by 27.8ms low signal. The interframe space is 17.5ms low 
   signal. This is repeated 4 times so that the receiver 
   catches at least one */
void send_code_boiler(unsigned char * bytecode1, unsigned char * bytecode2)
{
int i;
int x;
for (x = 0; x < 4u; x++)
{
	i = 0u;

	/* Send Start bit */
	SET_PORT(PORT_NUM);	
	DELAY_USECONDS(4543);				

	while(bytecode1[i] != '\0')
	{		
		/* Send bit 1 */
		if('1' == bytecode1[i])
		{
			CLR_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_LONG_BOILER);
			SET_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_SHORT_BOILER);
		}
		else /* Send bit 0 */
		{
			CLR_PORT(PORT_NUM);
			DELAY_USECONDS(PULSE_SHORT_BOILER);						
			SET_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_SHORT_BOILER);						
		}
		i++;
	}

	/* Send Sync bits */
	CLR_PORT(PORT_NUM);	
	DELAY_USECONDS(28000u);
	
	i = 0u;			

	while(bytecode2[i] != '\0')
	{		
		/* Send bit 1 */
		if('1' == bytecode2[i])
		{
			CLR_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_LONG_BOILER);
			SET_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_SHORT_BOILER);
		}
		else /* Send bit 0 */
		{
			CLR_PORT(PORT_NUM);
			DELAY_USECONDS(PULSE_SHORT_BOILER);						
			SET_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_SHORT_BOILER);						
		}
		i++;
	}

	/* Send Sync bits */
	CLR_PORT(PORT_NUM);	
	DELAY_USECONDS(17500u);			


}
}

int main(int argc, char *argv[])
{
int fp, fp1;
void * gpio_map;
void * stm_base; 
unsigned char * bytecode_on1;
unsigned char * bytecode_on2;
unsigned char * bytecode_off1;
unsigned char * bytecode_off2;
unsigned int min;

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


bytecode_on1 = "100100001111011000000000100000001"; /* 1 - 0x21EC0101 */
bytecode_on2 = "111011110000100111111111011111110";

bytecode_off1 = "100100001111011000000000100000000"; /* 1 - 0x21EC0100 */
bytecode_off2 = "111011110000100111111111011111111";

min = atoi(argv[1u]);

if (min == 1)
{
	/* Boiler ON */
	send_code_boiler(bytecode_on1, bytecode_on2);
}
else
{
	/* Boiler OFF */
	send_code_boiler(bytecode_off1, bytecode_off2);
}

CLR_PORT(PORT_NUM);
INPORT(PORT_NUM);
}
