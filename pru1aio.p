.origin 0 // offset of the start of the code in PRU memory
.entrypoint START // program entry point, used by debugger only

#include "pru1aio.hp"

// Register allocations

#define tmp0  r1
#define tmp1  r2
#define tmp2  r3
#define tmp3  r4
#define tmp4  r5
#define tmp5  r6

#define adc_  r7
#define fifodata r8

#define heartbeat r9

#define locals r10

#define value r11
#define channel   r12

#define buffer_pointer  r13
#define buffer_status r14
#define DANGER_GYM_REG r15
#define buffer_position  r22

#define buffer_stride r16

#define read_count r17

#define debug_reg r20
#define guard_reg r21

#define half_position r23

#define write_mask r24
#define digital_out r25

.macro HEARTBEAT
	MOV tmp1, 0x1
	MOV tmp0, PRU_IEP_BASE			// Set IEP base
									// Clear status register if set
	SBBO tmp1, tmp0, PRU_IEP_CMP_STATUS, 4
	HEARTBEAT_LOOP:					// Read CMP0
		LBBO tmp1, tmp0, PRU_IEP_CMP_STATUS, 4
		QBBC HEARTBEAT_LOOP, tmp1, 0
.endm

.macro SETBUFFERSTATUS 
.mparam status
	MOV buffer_status, status
    SBCO buffer_status, CONST_PRUSHAREDRAM, 0, 4 
.endm

.macro SETARMSTATUS
.mparam offset, status
	MOV buffer_status, status
	SBCO buffer_status, CONST_PRUSHAREDRAM, offset, 2 
.endm

.macro INCREMENT_BUFFER_COUNT
	LBCO tmp0, CONST_PRUSHAREDRAM, PRU_RTA_BUFFER_COUNT, 4
	ADD tmp0, tmp0, 1
	SBCO tmp0, CONST_PRUSHAREDRAM, PRU_RTA_BUFFER_COUNT, 4
.endm

.macro THROW_SIGNAL
	MOV r31.b0, PRU1_ARM_INTERRUPT+16
.endm

.macro GUARD
.mparam debug_val
	MOV guard_reg, 8000
	MOV debug_reg, debug_val
	SBCO debug_reg, CONST_PRUSHAREDRAM, PRU_RTA_SCRATCH + 13, 1
	SBCO buffer_position, CONST_PRUSHAREDRAM, PRU_RTA_BUFFER_POSITION, 4
	
	QBLT NO_ACTION, guard_reg, buffer_position
	
	MOV debug_reg, 1
	SBCO debug_reg, CONST_PRUSHAREDRAM, PRU_RTA_SCRATCH + 12, 1
	THROW_SIGNAL 
	HALT

NO_ACTION:
.endm

.macro READADC
	MOV read_count, 0
	SBCO read_count, CONST_PRUSHAREDRAM, PRU_RTA_READ_COUNT, 4
	
    //Initialize buffer status (0: empty, 1: first buffer is ready, 2: second buffer is ready)
    SETBUFFERSTATUS 0x3
	
	LBCO half_position, CONST_PRUSHAREDRAM, PRU_RTA_BUFFER_BYTES, 4
	LSR half_position, half_position, 1
	
	// Clear FIFO0
    INITV:
		LBCO tmp0, CONST_PRUSHAREDRAM, PRU_RTA_SAMPLE_MODE, 1
		QBEQ CONFIGURE, tmp0, RTA_STOPPED
		QBEQ INITV, tmp0, RTA_READY
											//	Address of Buffer 1 
		MOV buffer_pointer, PRU_RTA_BUFFER_BASE
											//	Counting variable set to buffer size
        LBCO buffer_position, CONST_PRUSHAREDRAM, PRU_RTA_BUFFER_BYTES, 4
        SBCO buffer_position, CONST_PRUSHAREDRAM, PRU_RTA_BUFFER_POSITION, 4
    READ:
        HEARTBEAT
											// Load channel map
		MOV tmp0, 0							// Zero out tmp0
											// Load channel map into tmp0
		LBCO tmp0, CONST_PRUSHAREDRAM, PRU_RTA_CHANNEL_ENABLED, 1
		LSL tmp0, tmp0, 1					// Channel map is bits 1 - 8, so shift left 1 bit to create ping mask
											// Store channel mask for posterity
		SBCO tmp0, CONST_PRUSHAREDRAM, PRU_RTA_SCRATCH + 1, 2
		SBBO tmp0, adc_, STEPCONFIG, 4   	// write STEPCONFIG register (this triggers capture)
											// increment capture counter		
		LBCO tmp1, CONST_PRUSHAREDRAM, PRU_RTA_CHANNEL_COUNT, 1
		
	WAIT_FOR_FIFO0:							// Wait while ADC propagates the FIFO
		LBBO tmp0, adc_, FIFO0COUNT, 4		// Load fifocount from fifo0counter
		AND tmp0, tmp0, 0xFF				// Mask lowest 8 bits
		QBNE WAIT_FOR_FIFO0, tmp0, tmp1		// Wait until there are channel count values in FIFO0

	READ_ALL_FIFO0:  
		LBBO value, fifodata, 0, 4			// Load data from FIFO0 Register
		MOV tmp1, 0xF0000					// Channel number mask
		AND tmp1, tmp1, value				// Mask the channel
		LSR tmp1, tmp1, 4					// Move channel bits into the high nibble of the lower dword
		MOV tmp2, 0xFFF						// ADC Mask
		AND value, value, tmp2				// Mask lower 12 bits of value
		OR value, tmp1, value				// Create a single dword from channel & reading
											// Write ADC value to buffer
        SBCO value, CONST_PRUSHAREDRAM, buffer_pointer, 2
											// Increment buffer pointer to next dword
        ADD buffer_pointer, buffer_pointer, 2
											// Decrement the buffer counter
		SUB tmp0, tmp0, 1					// Decrement fifo reads & buffer_position
		QBNE READ_ALL_FIFO0, tmp0, 0		// Repeat FIFO0 Read if there are more channels
		
											// Write Digitial In to buffer
		SBCO r31.w0, CONST_PRUSHAREDRAM, buffer_pointer, 2
											// Increment buffer pointer to next dword
        ADD buffer_pointer, buffer_pointer, 2
		
											// decrement count by the number of values read
        SUB buffer_position, buffer_position, buffer_stride
		SBCO buffer_position, CONST_PRUSHAREDRAM, PRU_RTA_BUFFER_POSITION, 4
		
		LSR tmp0, buffer_stride, 1			// Take half the memory decrement
											// Increment total read count
		ADD read_count, read_count, 1
											// Write read count to read counter memory
		SBCO read_count, CONST_PRUSHAREDRAM, PRU_RTA_READ_COUNT, 4 
		
		/*
			Read and write DIO
		*/
		LBCO write_mask, CONST_PRUSHAREDRAM, PRU_RTA_WRITE_MASK, 4
		QBEQ NO_DIGITAL_WRITE, write_mask, 0
		NOT tmp0, write_mask				
		AND tmp0, r30, tmp0					// Preserve unwritten bits in r30
		LBCO digital_out, CONST_PRUSHAREDRAM, PRU_RTA_DIGITAL_OUT, 4
		AND tmp1, write_mask, digital_out	// Set bit states for write mask
		OR r30, tmp1, tmp0					// Write bits to r30
		MOV write_mask, 0					// Prevents multiple writes of the same signal
		SBCO write_mask, CONST_PRUSHAREDRAM, PRU_RTA_WRITE_MASK, 4
		
	NO_DIGITAL_WRITE:
		
											// If count == half buffer left, then Buffer 1 is full & ready
        QBEQ CHBUFFSTATUS1, buffer_position, half_position
											// If count == 0, then Buffer 2 is full & ready
        QBEQ CHBUFFSTATUS2, buffer_position, 0

        QBA READ

    //	Buffer is full and ready, set buffer ready state
    CHBUFFSTATUS1:
        SETBUFFERSTATUS 0x1DEAD1
		INCREMENT_BUFFER_COUNT
											// Signal interrupt event 1
		THROW_SIGNAL 
        QBA READ

    //Change buffer status to 2
    CHBUFFSTATUS2:
        SETBUFFERSTATUS 0x2DEAD2
		INCREMENT_BUFFER_COUNT
											// Signal interrupt event 1
		THROW_SIGNAL
        QBA INITV

    //Send event to host program
    //MOV r31.b0, PRU0_ARM_INTERRUPT+16 
    HALT
	
.endm

// Starting point
START:
    // Enable OCP master port
    LBCO r0, CONST_PRUCFG, 4, 4		// Load 4 bytes from offset CONS_PRUCFG + 4
    CLR r0, r0, 4					// Set to 0
    SBCO r0, CONST_PRUCFG, 4, 4		// Store it (enables port)
									// Store some often-used constants
	MOV adc_, ADC_BASE				// Base memory address of TSC_ADC_SS Registers
	MOV fifodata, ADC_FIFO0DATA		// Base memory address of ADC FIFO0

    // C28 will point to 0x00012000 (PRU shared RAM)
    // C28 will point to 0x00012000 (PRU shared RAM)
    MOV r0, 0x00000120				// Magic for sharing memory ... I think this should be 0x00000100
    MOV tmp0, PRU1_CTRL + CTPPR_0	// Move programmable pointer to r0
    ST32 r0, tmp0					// Store r0 value to that pointer ... whatever.  I copied this.
	
CONFIGURE:
	
	/*
		Enable the IEP Counter with CMP0 reset
		This will act as our heartbeat to let us know that we should service
		an ADC call
		
		Because there is a lot of branching, our ADC processing code runs in a small
		range of time--theshortest is 96, longest is 101, so we have to suffer with at least 25ns skew
		(not counting hearbeat monitoring, defined below, probably another 5-10ns of skew, and
		another 3 clocks in processing per sample).
		
		This doesn't include averaging and sample delay (both of which are set to 16 cycles).
		Samples are taken linearly (not simultaneously), which eats +2048 cycles.  The total processing 
		time for all 8 channels is at least 2048 x 5ns + ~100 x 5ns = ~10740ns.  Maximum capture frequency 
		is 93100ksps (probably a bit less), or 1 sample for every 2148 PRU/ADC clocks (definitionally).
		
		This code expects the primary ARM CPU to respond to a buffer call every 100 samples.  This
		includes a system interrupt.  This means while we could safely call 50Ksps with this PRU code,
		it's not clear if the ARM system can handle 500 interrupts/second.  40-50 interrupts/second
		is probably a reasonable upper bound to do anything useful with, so 4-5Ksps is probably a reasonable
		upper bound.  Running the code "unfettered" results in around 6150 samples/second.
		
		If we put a fixed delay in, the skew would compound over time, making timing very hard.
		
		Instead, we use a watchdog timer using the IEP CMP0 register to determine when to fire an
		event.  Sample timings:
		
		5Ksps		40,000 counts
		2Ksps		100,000 counts
		1Ksps		200,000 counts
		500sps		400,000 counts
		100sps		2,000,000 counts
	*/
	MOV tmp0, PRU_IEP_BASE			// Set IEP base
									// Fetch global config
	MOV tmp1, 0x11					// Count of 1, enable counter
	SBBO tmp1, tmp0, PRU_IEP_GLOBAL_CFG, 4
	MOV tmp1, 0x3					// Enable counter compare 0, and counter reset
	SBBO tmp1, tmp0, PRU_IEP_CMP_CFG, 4
//	MOV tmp1, 100000				// Configure counter compare at 20 frames/second
	LBCO tmp1, CONST_PRUSHAREDRAM, PRU_RTA_IEP_CLOCK_COUNT, 4
	SBBO tmp1, tmp0, PRU_IEP_CMP0, 4
	
	// Disable ADC
	LBBO tmp0, adc_, CONTROL, 4		// Load control into tmp0
	MOV  tmp1, 0x1					// store 0x1 in tmp1
	NOT  tmp1, tmp1					// bitmask tmp1
	AND  tmp0, tmp0, tmp1			// turn off bit 0 in tmp0
	SBBO tmp0, adc_, CONTROL, 4		// disable ADC

	// Put ADC capture to its full speed
	MOV tmp0, 0
	SBBO tmp0, adc_, SPEED, 4
	
	// Configure STEPCONFIG registers for all 8 channels
	MOV tmp0, STEP1					// Store address of STEP1 register
	MOV tmp1, 0						// Current channel
	MOV tmp2, 0						// clear tmp2
									// Move SOC delay into tmp2
	LBCO tmp2, CONST_PRUSHAREDRAM, PRU_RTA_SAMPLE_SOC, 1
	LSL tmp2, tmp2, 24				// Shift into upper bits
//	MOV tmp2, 15 << 24				// Delay setting

	LBCO tmp4, CONST_PRUSHAREDRAM, PRU_RTA_CHANNEL_COUNT, 1
	LBCO buffer_stride, CONST_PRUSHAREDRAM, PRU_RTA_CHANNEL_COUNT, 1
	ADD buffer_stride, buffer_stride, 1
	LSL buffer_stride, buffer_stride, 1
	SBCO buffer_stride, CONST_PRUSHAREDRAM, PRU_RTA_SCRATCH, 1
	LBCO tmp5, CONST_PRUSHAREDRAM, PRU_RTA_CHANNEL_ENABLED, 1
	
FILL_STEPS:
	// Average 16 samples - 	10
	// Mode 0					0
	QBBC INCREMENT_CHANNEL, tmp5, 0
	LSL tmp3, tmp1, 19				// Store channel number in proper bit location [19..22] in tmp3 from tmp1
									// Store average in lower 8 bits
	LBCO tmp3.b0, CONST_PRUSHAREDRAM, PRU_RTA_SAMPLE_AVERAGE, 1
	//OR tmp3, tmp3, 0x10				// Average 16 samples
	SBBO tmp3, adc_, tmp0, 4		// Store configuration from tmp3 into STEPn
	ADD tmp0, tmp0, 4				// Increment step register
	SBBO tmp2, adc_, tmp0, 4		// Set STEPn delay to tmp2
	ADD tmp0, tmp0, 4				// Increment STEP Config
INCREMENT_CHANNEL:
	LSR tmp5, tmp5, 1
	ADD tmp1, tmp1, 1				// Increment channel
	QBLT FILL_STEPS, tmp1, tmp4		// Loop for all channels

	// Enable ADC with the desired mode (make STEPCONFIG registers writable, use tags, enable)
	LBBO tmp0, adc_, CONTROL, 4		// Fetch current ADC control
	OR   tmp0, tmp0, 0x7			// Set enable bits
	SBBO tmp0, adc_, CONTROL, 4		// Enable ADC
	
	// END OF CONFIGURATION, WAIT FOR START-OF-CAPTURE
	LBCO tmp0, CONST_PRUSHAREDRAM, PRU_RTA_SAMPLE_MODE, 1
	QBEQ CONFIGURE, tmp0, RTA_STOPPED

WAIT_READY:
	// END OF CONFIGURATION, WAIT FOR START-OF-CAPTURE
	LBCO tmp0, CONST_PRUSHAREDRAM, PRU_RTA_SAMPLE_MODE, 1
	QBEQ WAIT_READY, tmp0, RTA_READY
	
    //Read ADC and FIFOCOUNT
    READADC