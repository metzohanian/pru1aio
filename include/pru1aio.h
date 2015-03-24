/******************************************************************************
* Include Files                                                               *
******************************************************************************/
#ifndef PRU_RTA_H
#define PRU_RTA_H

// Standard header files
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
//#include <time.h>
//#include <sys/time.h>

// Driver header file
#include <pruss/prussdrv.h>
#include <pruss/pruss_intc_mapping.h>

/******************************************************************************
* Local Macro Declarations                                                    * 
******************************************************************************/
#define PRU_NUM0	  0
#define PRU_NUM1	  1
#define OFFSET_SHAREDRAM 0x2000		//equivalent with 0x00002000

#define PRUSS1_SHARED_DATARAM    4

#define CAPTURE_TIME_MS 1000

#define RTA_STOPPED 0
#define RTA_READY 1
#define RTA_CONTINUOUS 2

/******************************************************************************
* Type Declarations                                                           * 
******************************************************************************/
typedef struct __attribute__ ((__packed__)) s_pru_rta_readings {
	unsigned char buffer;
	unsigned short readings[8];
	unsigned short digital_in;
} pru_rta_readings;

typedef struct __attribute__ ((__packed__)) s_pru_shared_control {
	unsigned int current_buffer;			// 0x0		Complete
	unsigned int buffer_count;				// 0x4		Complete
	unsigned char buffer_size;				// 0x8		Complete
	unsigned char channel_enabled_mask;		// 0x9		Complete
	unsigned char sample_average;			// 0xA		Complete
	unsigned char sample_soc;				// 0xB		Complete
	unsigned int sample_rate;				// 0xC		Complete
	unsigned char sample_mode;				// 0x10		Complete
	unsigned char channel_count;			// 0x11		Complete
	unsigned int buffer_memory_bytes;		// 0X12		Complete
	unsigned int buffer_position;			// 0X16		Complete
	unsigned int read_count;				// 0x1A		Complete
	unsigned int iep_clock_count;			// 0x1E
	unsigned int write_mask;				// 0x22
	unsigned int digital_out;				// 0x26
	unsigned char scratch[14];				// 0x2A
} pru_shared_control;

typedef enum e_CONDITION {
	greater = 0,
	greater_eq,
	less,
	less_eq,
	equal,
	rising_edge,
	falling_edge
} comparator;

typedef enum e_SIGNAL {
	CHANNEL_0 = 0,
	CHANNEL_1,
	CHANNEL_2,
	CHANNEL_3,
	CHANNEL_4,
	CHANNEL_5,
	CHANNEL_6,
	CHANNEL_7,
	CHANNEL_DIO
} input_signal;

typedef enum e_TRIGGER_STATE {
	NOT_TRIGGERED = 0,
	TRIGGERED
} trigger_state;

typedef struct __attribute__ ((__packed__)) s_pru_shared_mem {
	pru_shared_control control;
	unsigned short *buffer_1;
	unsigned short *buffer_2;
} pru_shared_mem;

typedef struct __attribute__ ((__packed__)) s_pru_rta_condition {
	char name[33];
	comparator condition;
	input_signal signal;
	unsigned short comp1;
	unsigned short comp2;
	
	int last_signal;
	
	trigger_state triggered;
	unsigned short trigger_count;
} pru_rta_condition;

typedef struct __attribute__ ((__packed__)) s_pru_rta_conditions {
	char number_conditions;
	pru_rta_condition *conditions;
} pru_rta_conditions;

typedef struct __attribute__ ((__packed__)) s_pru_rta_call_state {
	pru_rta_readings *readings;
	pru_rta_readings buffer_mean;
	int records;
	int maximum_records;
	int signals;
	void *caller_state;
	pru_rta_conditions *conditions;
} pru_rta_call_state;

/******************************************************************************
* External Function declarations                                              * 
******************************************************************************/
void pru_rta_configure(pru_shared_mem *pru_mem);
pru_shared_mem *pru_rta_init();

void pru_rta_clear_pru(int PRU);

void pru_rta_start_firmware();

void pru_rta_start_capture(pru_shared_mem *pru_mem, 
		pru_rta_readings buffer[],
		pru_rta_call_state* call_state, 
		void (*async_call)(unsigned int buffer_count, unsigned short buffer_size, pru_rta_readings *captured_buffer, pru_rta_call_state* call_state, pru_shared_mem *pru_mem));
void pru_rta_stop_capture(pru_shared_mem *pru_mem);

void print_pru_map(pru_shared_mem *pru_mem);
void print_pru_map_address(pru_shared_mem *pru_mem);

pru_rta_call_state *pru_rta_init_call_state();
void pru_rta_free_call_state(pru_rta_call_state *call_state);

void pru_rta_add_condition(pru_rta_conditions *conditions, char *name, comparator condition, input_signal signal, unsigned short comp1, unsigned short comp2);

pru_rta_readings *pru_rta_init_capture_buffer(pru_shared_mem *pru_mem);

/******************************************************************************
* Internal Function declarations                                              * 
******************************************************************************/
void pru_rta_set_digital_out(pru_shared_mem *pru_mem, unsigned int write_mask, unsigned int digital_out);

void pru_rta_process_conditions(int buffer_size, pru_rta_readings *current_readings, pru_rta_conditions *conditions);
int pru_rta_condition_is_triggered(pru_rta_readings reading, pru_rta_condition *condition);
pru_rta_conditions *pru_rta_init_conditions();
void pru_rta_destroy_conditions(pru_rta_conditions *conditions);

void (*pru_rta_async_capture)(unsigned int buffer_count, unsigned short buffer_size, pru_rta_readings *captured_buffer, pru_rta_call_state* call_state, pru_shared_mem *pru_mem);

unsigned short pru_rta_adc_value(unsigned short value);
unsigned short pru_rta_adc_channel(unsigned short value);

void pru_printf_hello(char* world);
void pru_rta_test_callback(pru_shared_mem *pru_mem, 
		pru_rta_readings* buffer,
		pru_rta_call_state* call_state, 
		void (*async_call)(unsigned int buffer_count, unsigned short buffer_size, pru_rta_readings *captured_buffer, pru_rta_call_state* call_state, pru_shared_mem *pru_mem));

#endif
