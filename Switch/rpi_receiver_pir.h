#ifndef RPI_PIR_RECEIVER_H

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
#define INPORT(a)               *(gpio + a/10) &= ~(unsigned int)(7<<((a%10)*3))
#define OUTPORT(a)              *(gpio + a/10) |= (unsigned int)(1<<((a%10)*3))
#define SET_PORT(a)             *(gpio + 7 + a/32) = (unsigned int)(1<<a)
#define CLR_PORT(a)             *(gpio + 10 + a/32) = (unsigned int)(1<<a)
#define DETECT_EDGE(a)          (*(gpio + 16 + a/32)>>a)
#define FALLING_EDGE(a)         *(gpio + 22 + a/32) = (unsigned int)(1<<a)
#define RISING_EDGE(a)          *(gpio + 19 + a/32) = (unsigned int)(1<<a)
#define EVENT_CLEAR(a)          *(gpio + 16 + a/32) = (unsigned int)(1<<a)
#define PIN_LEVEL(a)            (*(gpio + 13 + a/32)>>a)

#define PORT_NUM 23u

#define PIR1_CODE1 0xF8DA
#define PIR1_CODE2 0x1831
#define PIR2_CODE1 0x6416
#define PIR2_CODE2 0x042C

#define BELL_CODE 0x704
#define BYRON_CODE 0xB2C
#define NEXA_MS1_CODE1 0x5807
#define NEXA_MS1_CODE2 0xF689
#define NEXA_MS1_CODE3 0xF699

/* Tolerance percent */
#define T0 10
#define T1 20
#define T2 20
#define T3 20

/* Lower and upper limits */
#define TL0 (1 - ((float)T0/100))
#define TH0 (1 + ((float)T0/100))
#define TL1 (1 - ((float)T1/100))
#define TH1 (1 + ((float)T1/100))
#define TL2 (1 - ((float)T2/100))
#define TH2 (1 + ((float)T2/100))
#define TL3 (1 - ((float)T3/100))
#define TH3 (1 + ((float)T3/100))

/* Enable/disable protocols */
#define P1_ENABLED 1
#define P2_ENABLED 0
#define P3_ENABLED 1
#define P4_ENABLED 1

#define U32_MAX 4294967295uL

/* Supported protocols */
typedef enum
{
PIR,
BELL,
BYRON,
NEXA,

MAX_PROTOCOL
}protocol_t;

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
}pulse_t;

#endif
