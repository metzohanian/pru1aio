// Standard header files
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// Driver header file
#include <pruss/prussdrv.h>
#include <pruss/pruss_intc_mapping.h>

#include "pru1aio.h"

pru_shared_mem *pru_memory;

/******************************************************************************
* Main                                                                        * 
******************************************************************************/

void do_buffer_work(unsigned int buffer_count, unsigned short buffer_size, pru_rta_readings *captured_buffer, pru_rta_call_state *call_state, pru_shared_mem *pru_mem) {
	for (int i = 0; i < buffer_size; i++) {
		call_state->readings[call_state->records].buffer = captured_buffer[i].buffer;
		for (int j = 0; j < pru_mem->control.channel_count; j++) {
			call_state->readings[call_state->records].readings[j] = captured_buffer[i].readings[j];
		}
		call_state->readings[call_state->records].digital_in = captured_buffer[i].digital_in;
		call_state->records++;
	}
	call_state->signals++;
	pru_rta_set_digital_out(pru_mem, 0x3, 0x3);
	if (call_state->records >= call_state->maximum_records)
		pru_rta_stop_capture(pru_mem);
	return;
}

int main (int argc, char* argv[])
{
	int capture_time_ms = CAPTURE_TIME_MS;
	pru_rta_call_state *call_state = malloc(sizeof(pru_rta_call_state));
	struct timeval start_time, end_time;

	if (argc < 4) {
		printf("Usage: %s <csv file> <capture time ms> <sample rate>\n", argv[0]);
		return(-1);
	}
	sscanf(argv[2], "%d", &capture_time_ms);	

    pru_memory = pru_rta_init();
	printf("Clear PRUs\n");
    pru_rta_clear_pru(PRU_NUM0);
    pru_rta_clear_pru(PRU_NUM1);
	
	// Configure RTA
	printf("Configure RTA\n");
	pru_memory->control.buffer_size = 40;
	pru_memory->control.channel_enabled_mask = 0x7F;
	pru_memory->control.sample_soc = 15;
	pru_memory->control.sample_average = 16;
	pru_memory->control.sample_rate = 2000;
	sscanf(argv[3], "%d", &(pru_memory->control.sample_rate));
	
	pru_rta_configure(pru_memory);
	
	print_pru_map_address(pru_memory);
	
	call_state->maximum_records = capture_time_ms / 1000.0 * pru_memory->control.sample_rate;
	call_state->readings = malloc(call_state->maximum_records * sizeof(pru_rta_readings));
	call_state->records = 0;
	call_state->signals = 0;
	call_state->caller_state = 0;
	
	call_state->conditions = pru_rta_init_conditions();
	printf("%x\n", (unsigned int)call_state->conditions);
	//(pru_rta_conditions *conditions, comparator condition, input_signal signal, unsigned short comp1, unsigned short comp2);
	pru_rta_add_condition(call_state->conditions, "Equals 3000", greater_eq, CHANNEL_0, 3000, 0);
	pru_rta_add_condition(call_state->conditions, "Equals 3100", equal, CHANNEL_0, 3100, 0);
	pru_rta_add_condition(call_state->conditions, "Equals 3200", less_eq, CHANNEL_0, 3200, 0);
	pru_rta_add_condition(call_state->conditions, "Equals 3300", equal, CHANNEL_0, 3300, 0);
	pru_rta_add_condition(call_state->conditions, "Equals 3400", greater_eq, CHANNEL_0, 3400, 0);
	pru_rta_add_condition(call_state->conditions, "Equals 3500", equal, CHANNEL_0, 3500, 0);
	pru_rta_add_condition(call_state->conditions, "Equals 3600", less_eq, CHANNEL_0, 3600, 0);
	pru_rta_add_condition(call_state->conditions, "Equals 3700", equal, CHANNEL_0, 3700, 0);
	pru_rta_add_condition(call_state->conditions, "Equals 3800", greater_eq, CHANNEL_0, 3800, 0);
	pru_rta_add_condition(call_state->conditions, "Equals 3900", equal, CHANNEL_0, 3900, 0);
	
	pru_rta_add_condition(call_state->conditions, "Equals 2000", less_eq, CHANNEL_0, 2000, 0);
	pru_rta_add_condition(call_state->conditions, "Equals 2100", equal, CHANNEL_0, 2100, 0);
	pru_rta_add_condition(call_state->conditions, "Equals 2200", greater_eq, CHANNEL_0, 2200, 0);
	pru_rta_add_condition(call_state->conditions, "Equals 2300", equal, CHANNEL_0, 2300, 0);
	pru_rta_add_condition(call_state->conditions, "Equals 2400", less_eq, CHANNEL_0, 2400, 0);
	pru_rta_add_condition(call_state->conditions, "Equals 2500", equal, CHANNEL_0, 2500, 0);
	pru_rta_add_condition(call_state->conditions, "Equals 2600", greater_eq, CHANNEL_0, 2600, 0);
	pru_rta_add_condition(call_state->conditions, "Equals 2700", equal, CHANNEL_0, 2700, 0);
	pru_rta_add_condition(call_state->conditions, "Equals 2800", less_eq, CHANNEL_0, 2800, 0);
	pru_rta_add_condition(call_state->conditions, "Equals 2900", equal, CHANNEL_0, 2900, 0);
	
	pru_rta_start_firmware();
	
	printf("Start Data Capture\n");
	
	gettimeofday(&start_time, NULL);
	pru_rta_start_capture(pru_memory, & do_buffer_work, call_state);
	gettimeofday(&end_time, NULL);
	
	float elapsed_time = end_time.tv_sec - start_time.tv_sec + (end_time.tv_usec - start_time.tv_usec ) / 1e6; 

	int num_channels = pru_memory->control.channel_count;
	float sample_rate = (float)pru_memory->control.sample_rate;
	
    printf("\tINFO: PRU completed transfer in %010g and %d signals\n\t\tBuffer count: %d.\r\n", elapsed_time, call_state->signals, pru_memory->control.buffer_count);
    prussdrv_pru_clear_event (PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);
	prussdrv_pru_clear_event(PRU_EVTOUT_1, PRU0_ARM_INTERRUPT);

	for (unsigned short *m = pru_memory->buffer_1; m < pru_memory->buffer_1 + 80; m += 8) {
		for (int unsigned short *n = m; n < m + 8; n++) {
			printf("%08X: %04x\t", (unsigned int)n, (unsigned short)n[0]);
		}
		printf("\n");
	}
	
	
    /* Disable PRU*/
    prussdrv_pru_disable(PRU_NUM1);
    prussdrv_exit();

	pru_rta_destroy_conditions(call_state->conditions);
	
	printf("Writing results to %s\n", argv[1]);

	FILE *fp = fopen(argv[1], "w");
	for (int l = 0; l < call_state->records; l++) {
		fprintf(fp, "%f,%x,", l / sample_rate, call_state->readings[l].buffer);
		for (int c = 0; c < num_channels; c++) {
			fprintf(fp, "%d", (unsigned int)(call_state->readings[l].readings[c]));
			if (c < num_channels-1)
				fprintf(fp, ",");
		}
		fprintf(fp, ",%04x\n", (unsigned int)call_state->readings[l].digital_in);
	}
	fclose(fp);
	
    return(0);
}
