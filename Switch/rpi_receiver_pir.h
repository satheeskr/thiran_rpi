#ifndef RPI_RECEIVER_PIR_H
#define RPI_RECEIVER_PIR_H

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

#define TRUE 1u
#define FALSE 0u

#define PERIPHERAL_BASE (0x3F000000)
#define GPIO_BASE (PERIPHERAL_BASE + 0x200000)
#define STM_BASE (PERIPHERAL_BASE + 0x3000)
#define INPORT(a)               *(gpio + a/10) &= ~(unsigned int)(7<<((a%10)*3))
#define OUTPORT(a)              *(gpio + a/10) |= (unsigned int)(1<<((a%10)*3))
#define SET_PORT(a)             *(gpio + 7 + a/32) = (unsigned int)(1<<a)
#define CLR_PORT(a)             *(gpio + 10 + a/32) = (unsigned int)(1<<a)
#define DETECT_EDGE(a)          (*(gpio + 16 + a/32)>>a)
#define FALLING_EDGE(a)         *(gpio + 22 + a/32) = (unsigned int)(1<<a)
#define RISING_EDGE(a)          *(gpio + 19 + a/32) = (unsigned int)(1<<a)
#define EVENT_CLEAR(a)          *(gpio + 16 + a/32) = (unsigned int)(1<<a)
#define PIN_LEVEL(a)            (*(gpio + 13 + a/32)>>a)

#define PORT_NUM 24u

#define PIR1_CODE1 0xF8DA
#define PIR1_CODE2 0x1831
#define PIR1_CODE3 0x2200

#define PIR2_CODE1 0x6416
#define PIR2_CODE2 0x042C
#define PIR2_CODE3 0x2200

#define PIR3_CODE1 0x125E
#define PIR3_CODE2 0x2E8F
#define PIR3_CODE3 0x2200

#define PIR4_CODE1 0x2F76
#define PIR4_CODE2 0x61F4
#define PIR4_CODE3 0x2200

#define PIR5_CODE1 0x2F76
#define PIR5_CODE2 0x61F4
#define PIR5_CODE3 0x2200

#define BELL_CODE      0x0704
#define BYRON_CODE     0x0B2C

#define SMOKE_CODE1    0x5502
#define SMOKE_CODE2    0x0006

#define NEXA_MS1_CODE1 0x5807
#define NEXA_MS1_CODE2 0xF689
#define NEXA_MS1_CODE3 0xF699

/* Tolerance percent */
#define T1 20
#define T2 20
#define T3 20
#define T4 20
#define T5 20

/* Lower and upper limits */
#define TL1 (1 - ((float)T1/100))
#define TH1 (1 + ((float)T1/100))
#define TL2 (1 - ((float)T2/100))
#define TH2 (1 + ((float)T2/100))
#define TL3 (1 - ((float)T3/100))
#define TH3 (1 + ((float)T3/100))
#define TL4 (1 - ((float)T4/100))
#define TH4 (1 + ((float)T4/100))
#define TL5 (1 - ((float)T5/100))
#define TH5 (1 + ((float)T5/100))

/* Enable/disable protocols */
#define P1_ENABLED 0
#define P2_ENABLED 0
#define P3_ENABLED 1
#define P4_ENABLED 1
#define P5_ENABLED 1

#define U32_MAX 4294967295uL
#define SLEEP_TIME1  2u
#define SLEEP_TIME2 60u

/* Supported protocols */
typedef enum
{
PIR,
BELL,
BYRON,
SMOKE,
NEXA,

MAX_PROTOCOL
}protocol_t;

/* Number of PIRs */
typedef enum
{
PIR1 = 0u,
PIR2,
PIR3,
PIR4,

MAX_NUM_PIR
}pir_list_t;

/* PIR code */
typedef struct
{
	unsigned long code1;
	unsigned long code2;
	unsigned long code3;
}pir_t;

/* Protocol configuration */
typedef struct
{
        unsigned char enabled;
        unsigned int start_min;
        unsigned int start_max;
        unsigned int short_min;
        unsigned int short_max;
        unsigned int long_min;
        unsigned int long_max;
        unsigned int if_gap_min;
        unsigned int if_gap_max;
        unsigned int preamble_min;
        unsigned int preamble_max;
        unsigned int max_bits;
        unsigned int start_bit;
        unsigned int end_bit;

}pulse_t;

#endif
