# pru1aio
BeagleBone Black PRU 1 Data Capture Library

This is intended to be a library to implement continuous analog and digital capture via the PRU (PRU1, specifically) for the BeagleBone Black.

The code is largely a rewrite of code found on Google Groups (almost entirely rewritten, actually), but shares some concepts. It is also a partial re-implementation of the C# National Instruments DAQmx library (namely, the asynchronous callback).

## Features:
 - Capture continuously on up to 7 analog channels and 16 digital channels at up to 20Ksps
 - Up to 31 bits of digital out (via r30)
 - Low skew, the trigger for capturing data is based off of the IEP timer, rather than spinlocks; this means you can program to capture data for long periods of time (hours, days, or months) having almost zero long-term skew.  
 - There is a small amount of jitter in samples, based on specific logic flow, but the jitter is on the order of several nanoseconds to ~2 microseconds.  
 - The interchannel skew of the analog inputs is configurable inversely with fidelity via the sample_average and sample_soc parameters (fidelity and skew are inversely related in the PRU).
 - Digital input has no programmatic skew.
 - Up to 20 per-channel conditional signals triggers based on buffered data (>, >=, <, <=, digital/rising edge, digital/falling edge).
 - Configurable buffer size/callback rate, double-buffered.
 - Start/Pause/Stop/Reset the firmware.

It is currently a work-in-progress, and not ready for use to speak of.

## Progress:

 - Conversion to a proper library (.so) [Complete but I don't know what I'm doing]
 - Call-back architecture to support asyncrhonous, continuous, frame-based processing of AIO and DIO data [Complete]
 - Library-based conditional checking of data [Complete]
 - Sample use code [Complete(ish)]
 - Low-skew, fully programmable capture rate in Samples/Second [Complete]
 - Programmable fidelity for AIO [Complete]
 - Programmable AI channel profile [Complete]
 - Programmable DIOs (up to 16 In/31 Out) [Complete]
 - Ring Buffer [In Progress]
 - C# wrapper [Complete]
 - Rust wrapper [Not Started]
 - Proper documentation [Not Started]
 - Start/Pause/Stop/Reset the firmware. [Complete]
 
 ## Usage
 
  - Several examples are in main.c
  - In general, you will need a callback function with a signature as follows: void callback(unsigned int buffer_count, unsigned short buffer_size,
  ```c
pru_rta_readings *captured_buffer, pru_rta_call_state *call_state, pru_shared_mem *pru_mem);
	
pru_rta_call_state *call_state = malloc(sizeof(pru_rta_call_state));
pru_shared_mem *pru_memory;
	
pru_memory = pru_rta_init();
pru_rta_clear_pru(PRU_NUM0);
pru_rta_clear_pru(PRU_NUM1);

pru_memory->control.buffer_size = 40;
pru_memory->control.channel_enabled_mask = 0x7F;	// Which AIN channels to init
pru_memory->control.sample_soc = 15;				// SOC is the number of cycles to take each sample
/**********************
 *    Sample Average is the number of samples to average to take each sample
 *    Inter-sample jitter is 5ns x sammple_soc x sample_average
 *    Sample rate is no more than 1,000,000,000 / (5 * sample_soc * sample_average *  __builtin_popcount(channel_enabled_mask)
 *********************/
pru_memory->control.sample_average = 16;
pru_memory->control.sample_rate = 2000;	

pru_rta_configure(pru_memory);
	
call_state->conditions = pru_rta_init_conditions();
	
pru_rta_start_firmware();
	
pru_rta_start_capture(pru_memory, & callback, call_state);
	
prussdrv_pru_clear_event(PRU_EVTOUT_0, PRU0_ARM_INTERRUPT);
prussdrv_pru_clear_event(PRU_EVTOUT_1, PRU0_ARM_INTERRUPT);
	
prussdrv_pru_disable(PRU_NUM1);
prussdrv_exit();

pru_rta_destroy_conditions(call_state->conditions);
```

## Known Issues
 - Segfaults on first run.
