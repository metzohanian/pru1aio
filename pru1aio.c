#include "pru1aio.h"

void pru_rta_clear_pru(int PRU) {
	prussdrv_pru_enable(PRU);
    prussdrv_exec_program (PRU, "./clearpru.bin");
	prussdrv_pru_disable(PRU);
}

void pru_rta_start_firmware() {
    pru_rta_clear_pru(PRU_NUM1);
	prussdrv_pru_enable(PRU_NUM1);
    prussdrv_exec_program (PRU_NUM1, "./pru1aio.bin");
}

pru_rta_conditions *pru_rta_init_conditions() {
	pru_rta_conditions *conditions;
	conditions = malloc(sizeof(pru_rta_conditions));
	conditions->number_conditions = 0;
	conditions->conditions = malloc(sizeof(pru_rta_condition) * 20);
	return conditions;
}

void pru_rta_destroy_conditions(pru_rta_conditions *conditions) {
	free(conditions->conditions);
	free(conditions);
}

void pru_rta_add_condition(pru_rta_conditions *conditions, char* name, comparator condition, input_signal signal, unsigned short comp1, unsigned short comp2) {
	if (conditions->number_conditions == 20)
		return;
	strncpy(conditions->conditions[(int)conditions->number_conditions].name, name, 32);
	conditions->conditions[(int)conditions->number_conditions].condition 	= 	condition;
	conditions->conditions[(int)conditions->number_conditions].signal 		= 	signal;
	conditions->conditions[(int)conditions->number_conditions].comp1 		= 	comp1;
	conditions->conditions[(int)conditions->number_conditions].comp1 		= 	comp1;
	
	conditions->conditions[(int)conditions->number_conditions].triggered 	= 	NOT_TRIGGERED;
	conditions->conditions[(int)conditions->number_conditions].trigger_count = 	0;
	
	conditions->number_conditions++;
}

void pru_rta_process_conditions(int buffer_size, pru_rta_readings *readings, pru_rta_conditions *conditions) {
	for (int c = 0; c < conditions->number_conditions; c++) {
		for (int r = 0; r < buffer_size; r++) {
			if (pru_rta_condition_is_triggered(readings[r], &(conditions->conditions[c]))) {
				conditions->conditions[c].triggered = TRIGGERED;
				conditions->conditions[c].trigger_count++;
//				printf("Trigger: %s %d\n", conditions->conditions[c].name, conditions->conditions[c].trigger_count);
				break;
			}
		}
	}
}

int pru_rta_condition_is_triggered(pru_rta_readings reading, pru_rta_condition *condition) {
	int signal;
	switch(condition->signal) {
		case CHANNEL_0:   signal = reading.readings[0]; break;
		case CHANNEL_1:   signal = reading.readings[1]; break;
		case CHANNEL_2:   signal = reading.readings[2]; break;
		case CHANNEL_3:   signal = reading.readings[3]; break;
		case CHANNEL_4:   signal = reading.readings[4]; break;
		case CHANNEL_5:   signal = reading.readings[5]; break;
		case CHANNEL_6:   signal = reading.readings[6]; break;
		case CHANNEL_7:   signal = reading.readings[7]; break;
		case CHANNEL_DIO: signal = reading.digital_in & (0x1 << condition->comp2); break;
	}
	trigger_state triggered = NOT_TRIGGERED;
	switch (condition->condition) {
		case greater       : if (signal >  condition->comp1) triggered = TRIGGERED; break;
		case greater_eq    : if (signal >= condition->comp1) triggered = TRIGGERED; break;
		case less          : if (signal <  condition->comp1) triggered = TRIGGERED; break;
		case less_eq       : if (signal <= condition->comp1) triggered = TRIGGERED; break;
		case equal         : if (signal == condition->comp1) triggered = TRIGGERED; break;
		case rising_edge   : if (signal == 1 && condition->last_signal == 0) triggered = TRIGGERED; break;
		case falling_edge  : if (signal == 0 && condition->last_signal == 1) triggered = TRIGGERED; break;
	}
	if (triggered == TRIGGERED) {
		condition->triggered = TRIGGERED;
		condition->triggered++;
	}
	condition->last_signal = signal;
	return triggered;
}

void pru_rta_set_digital_out(pru_shared_mem *pru_mem, unsigned int write_mask, unsigned int digital_out) {
	pru_mem->control.digital_out = digital_out;
	pru_mem->control.write_mask = write_mask;
}

void pru_rta_start_capture(
		pru_shared_mem *pru_mem, 
		void (*async_call)(unsigned int buffer_count, unsigned short buffer_size, pru_rta_readings *captured_buffer, pru_rta_call_state* call_state, pru_shared_mem *pru_mem), 
		pru_rta_call_state* call_state) {
		
	unsigned short sample_value = 0;
	int buffer_count = 0;
	int target_buff = 1;
	int signals = 0;
	int current_buffer = 0;
	pru_rta_readings *current_readings = malloc(pru_mem->control.buffer_size * sizeof(pru_rta_readings));
	
	pru_rta_async_capture = async_call;
	pru_mem->control.sample_mode = RTA_CONTINUOUS;
	do {
		buffer_count = 0;
		prussdrv_pru_wait_event(PRU_EVTOUT_1);
		prussdrv_pru_clear_event(PRU_EVTOUT_1, PRU1_ARM_INTERRUPT);

		current_buffer = pru_mem->control.buffer_count;
		signals++;

		if (signals != current_buffer)
			printf("Dropped buffer! %d %d\n", signals, current_buffer);
		target_buff = 0;
		if (pru_mem->control.current_buffer == 0x1DEAD1){ // First buffer is ready
			target_buff = 1;
		} else if(pru_mem->control.current_buffer == 0x2DEAD2){ // Second buffer is ready
			target_buff = 2;
		}
		if (target_buff != 1 && target_buff != 2)
			continue;
		for(int i=0; i < pru_mem->control.buffer_size * (pru_mem->control.channel_count + 1); i+=pru_mem->control.channel_count + 1) {
			current_readings[buffer_count].buffer = target_buff;
			// Read Analog values
			for (int j = i; j < i + pru_mem->control.channel_count; j++) {
				if (target_buff == 1) {
					sample_value = pru_mem->buffer_1[j];
				} else if (target_buff == 2) {
					sample_value = pru_mem->buffer_2[j];
				} else {
					sample_value = 0xFF;
				}
				current_readings[buffer_count].readings[pru_rta_adc_channel(sample_value)] = pru_rta_adc_value(sample_value);
			}
			// Read digital values
			if (target_buff == 1) {
				sample_value = pru_mem->buffer_1[i + pru_mem->control.channel_count];
			} else if (target_buff == 2) {
				sample_value = pru_mem->buffer_2[i + pru_mem->control.channel_count];
			} else {
				sample_value = 0xDEAD;
			}
			current_readings[buffer_count].digital_in = sample_value;
			
			buffer_count++;
		}
		pru_rta_process_conditions(pru_mem->control.buffer_size, current_readings, call_state->conditions);
		(*pru_rta_async_capture)(current_buffer, pru_mem->control.buffer_size, current_readings, call_state, pru_mem);
	} while (pru_mem->control.sample_mode == RTA_CONTINUOUS);
	
	free(current_readings);
}

void pru_rta_pause_capture(pru_shared_mem *pru_mem) {
	pru_mem->control.sample_mode = RTA_READY;
}

void pru_rta_stop_capture(pru_shared_mem *pru_mem) {
	pru_mem->control.sample_mode = RTA_STOPPED;
}

pru_shared_mem *pru_rta_init() {
	void* shared_memory;
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;

    prussdrv_init();

    prussdrv_open(PRU_EVTOUT_1);
	
    prussdrv_pruintc_init(&pruss_intc_initdata);
    prussdrv_map_prumem(PRUSS0_SHARED_DATARAM, &shared_memory);
	
	prussdrv_pru_clear_event(PRU_EVTOUT_1, PRU1_ARM_INTERRUPT);
	
	pru_shared_mem *pru_memory = (pru_shared_mem*) (shared_memory + OFFSET_SHAREDRAM);
	pru_memory->control.sample_mode = RTA_STOPPED;
	
	return pru_memory;
}

void print_pru_map_address(pru_shared_mem *pru_mem) {
	printf("Base Address: %x\n", (unsigned int)pru_mem);
	unsigned int mem_base = (unsigned int)pru_mem;
	printf("\tcurrent_buffer: %x\n", (unsigned int)&(pru_mem->control.current_buffer) - mem_base);
	printf("\tbuffer_count: %x\n", (unsigned int)&(pru_mem->control.buffer_count) - mem_base);
	printf("\tbuffer_size: %x\n", (unsigned int)&(pru_mem->control.buffer_size) - mem_base);
	printf("\tchannel_enabled_mask: %x\n", (unsigned int)&(pru_mem->control.channel_enabled_mask) - mem_base);
	printf("\tsample_average: %x\n", (unsigned int)&(pru_mem->control.sample_average) - mem_base);
	printf("\tsample_soc: %x\n", (unsigned int)&(pru_mem->control.sample_soc) - mem_base);
	printf("\tsample_rate: %x\n", (unsigned int)&(pru_mem->control.sample_rate) - mem_base);
	printf("\tsample_mode: %x\n", (unsigned int)&(pru_mem->control.sample_mode) - mem_base);
	printf("\tchannel_count: %x\n", (unsigned int)&(pru_mem->control.channel_count) - mem_base);
	printf("\tbuffer_memory_bytes: %x\n", (unsigned int)&(pru_mem->control.buffer_memory_bytes) - mem_base);
	printf("\tbuffer_position: %x\n", (unsigned int)&(pru_mem->control.buffer_position) - mem_base);
	printf("\tread_count: %x\n", (unsigned int)&(pru_mem->control.read_count) - mem_base);
	printf("\tiep_clock_count: %x\n", (unsigned int)&(pru_mem->control.iep_clock_count) - mem_base);
	printf("\tscratch: %x\n", (unsigned int)&(pru_mem->control.scratch) - mem_base);
	printf("\tbuffer[0]: %x\n", (unsigned int)&(pru_mem->buffer_1) - mem_base);
	printf("\tbuffer[1]: %x\n", (unsigned int)&(pru_mem->buffer_2) - mem_base);
}

void print_pru_map(pru_shared_mem *pru_mem) {
	printf("Base Address: %x\n", (unsigned int)pru_mem);
	printf("\tcurrent_buffer: %x\n", (unsigned int)(pru_mem->control.current_buffer));
	printf("\tbuffer_count: %d\n", (unsigned int)(pru_mem->control.buffer_count));
	printf("\tbuffer_size: %d\n", (unsigned int)(pru_mem->control.buffer_size));
	printf("\tchannel_enabled_mask: %x\n", (unsigned int)(pru_mem->control.channel_enabled_mask));
	printf("\tsample_average: %d\n", (unsigned int)(pru_mem->control.sample_average));
	printf("\tsample_soc: %d\n", (unsigned int)(pru_mem->control.sample_soc));
	printf("\tsample_rate: %d\n", (unsigned int)(pru_mem->control.sample_rate));
	printf("\tsample_mode: ");
	switch ((unsigned int)(pru_mem->control.sample_mode)) {
		case RTA_STOPPED: printf("RTA_STOPPED\n");
			break;
		case RTA_READY: printf("RTA_READY\n");
			break;
		case RTA_CONTINUOUS: printf("RTA_CONTINUOUS\n");
			break;
	}
	printf("\tchannel_count: %d\n", (unsigned int)(pru_mem->control.channel_count));
	printf("\tbuffer_memory_bytes: %d\n", (unsigned int)(pru_mem->control.buffer_memory_bytes));
	printf("\tbuffer_position: %u\n", (unsigned int)(pru_mem->control.buffer_position));
	printf("\tread_count: %u\n", (unsigned int)(pru_mem->control.read_count));
	printf("\tiep_clock_count: %u\n", (unsigned int)(pru_mem->control.iep_clock_count));
	for (int i = 0; i < 14; i++)
		printf("\tscratch[%d]: %x\n", i, (unsigned int)(pru_mem->control.scratch[i]));
	
	printf("\tbuffer[0]: %x\n", (unsigned int)(pru_mem->buffer_1));
	printf("\tbuffer[1]: %x\n", (unsigned int)(pru_mem->buffer_2));
}

void pru_rta_configure(pru_shared_mem *pru_mem) {
	pru_mem->buffer_1 = (unsigned short*)(pru_mem) + sizeof(pru_shared_control)/sizeof(unsigned short) + (sizeof(unsigned short*)*2 / sizeof(unsigned short));
	pru_mem->buffer_2 = pru_mem->buffer_1 + pru_mem->control.buffer_size * (pru_mem->control.channel_count + 1);
	pru_mem->control.current_buffer = 0;
	pru_mem->control.buffer_count = 0;
	pru_mem->control.channel_count = (char)__builtin_popcount((unsigned int)pru_mem->control.channel_enabled_mask);
	pru_mem->control.buffer_memory_bytes = (pru_mem->control.channel_count + 1) * sizeof(unsigned short) * 2 * pru_mem->control.buffer_size;
	pru_mem->control.buffer_position = 0;
	pru_mem->control.iep_clock_count = 200000000 / pru_mem->control.sample_rate;
	pru_mem->control.read_count = 0;
	for (int i = 0; i < 14; i++)
		pru_mem->control.scratch[i] = 0;
	pru_mem->control.sample_mode = RTA_READY;
	sleep(1);		// Give configuration time to settle before launch -- this should be done with semaphores
}

unsigned short pru_rta_adc_channel(unsigned short value) {
	return (value >> 12) & 0xF;
}
/* 
 * FIFO0DATA register includes both ADC and channelID
 * so we need to remove the channelID
 */
unsigned short pru_rta_adc_value(unsigned short value)
{
	return value & 0xFFF;
}
