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
#define PULSE_SHORT 265u
#define PULSE_LONG (PULSE_SHORT * 5u)
#define PULSE_PREAMBLE (PULSE_SHORT * 10u)
#define PULSE_LONG (PULSE_SHORT * 5u)

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
void send_code(unsigned char * bytecode)
{
int i;
int x;
for (x = 0; x < 10; x++)
{
	i = 0;
	
	/* Pre-amble */
	SET_PORT(PORT_NUM);			
	DELAY_USECONDS(PULSE_SHORT);
	CLR_PORT(PORT_NUM);			
	DELAY_USECONDS(PULSE_PREAMBLE);
	while(bytecode[i] != '\0')
	{		
		/* Send bit 1 */
		if('1' == bytecode[i])
		{
			SET_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_SHORT);
			CLR_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_LONG);
			SET_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_SHORT);
			CLR_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_SHORT);
		}
		else /* Send bit 0 */
		{
			SET_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_SHORT);
			CLR_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_SHORT);
			SET_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_SHORT);
			CLR_PORT(PORT_NUM);			
			DELAY_USECONDS(PULSE_LONG);
		}
		i++;
	}

	/* Post-amble bits */
	SET_PORT(PORT_NUM);	
	DELAY_USECONDS(PULSE_SHORT);				
	CLR_PORT(PORT_NUM);
	DELAY_USECONDS((PULSE_SHORT * 38));			
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
	
/* Print instructions if no input received */
if (argc != 2)
{
	cmd = 255u;
}
else
{
	cmd = atoi(argv[1]);
}

if (cmd < 7)
{
	if (1 == cmd%2)
	{
		fprintf(stderr, "Turning ON switch %d\n", (cmd + 1)/2);
	}
	else if (0 == cmd)
	{
		fprintf(stderr, "Turning OFF all switches\n");
	}
	else
	{
		fprintf(stderr, "Turning OFF switch %d\n", cmd/2);
	}

}

switch(cmd)
{
case 1: /* Switch 1 on */
	bytecode = "00101110110001011011101110010000"; /* 0x2EC5BB90 */	
	break;
case 2: /* Switch 1 off */
	bytecode = "00101110110001011011101110000000"; /* 0x2EC5BB80 */
	break;
case 3: /* Switch 2 on */
	bytecode = "00101110110001011011101110010001"; /* 0x2EC5BB91 */
	break;
case 4: /* Switch 2 off */
	bytecode = "00101110110001011011101110000001"; /* 0x2EC5BB81 */
	break;
case 5: /* Switch 3 on */
	bytecode = "00101110110001011011101110010010"; /* 0x2EC5BB92 */
	break;
case 6: /* Switch 3 off */
	bytecode = "00101110110001011011101110000010"; /* 0x2EC5BB82 */
	break;
case 0: /* All Switches off */
	bytecode = "00101110110001011011101110100000"; /* 0x2EC5BBA0 */
	break;
case 55: /* Magnetic Switch Alarm */
	bytecode = "00101110110001011011101010111010"; /* 0x2EC5BABA */
        break;
default:
	printf("Enter 1 to turn ON switch 1\n");
	printf("Enter 2 to turn OFF switch 1\n");
	printf("Enter 3 to turn ON switch 2\n");
	printf("Enter 4 to turn OFF switch 2\n");
	printf("Enter 5 to turn ON switch 3\n");
	printf("Enter 6 to turn OFF switch 3\n");
	printf("Enter 0 to turn OFF all switches\n");							
	break;	
}

/* Transmit code to turn off/on the remote switch */
send_code(bytecode);

/* Ding-dong */
//send_code_bell("011100000100");
INPORT(PORT_NUM);
}
