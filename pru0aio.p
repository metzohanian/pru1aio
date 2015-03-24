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

#define write_mask r24
#define digital_out r25

// Starting point
START:
    // Enable OCP master port
    LBCO r0, CONST_PRUCFG, 4, 4		// Load 4 bytes from offset CONS_PRUCFG + 4
    CLR r0, r0, 4					// Set to 0
    SBCO r0, CONST_PRUCFG, 4, 4		// Store it (enables port)

    // C28 will point to 0x00012000 (PRU shared RAM)
    // C28 will point to 0x00012000 (PRU shared RAM)
    MOV r0, 0x00000120				// Magic for sharing memory ... I think this should be 0x00000100
    MOV tmp0, PRU0_CTRL + CTPPR_0	// Move programmable pointer to r0
    ST32 r0, tmp0					// Store r0 value to that pointer ... whatever.  I copied this.
	
WAIT_READY:
	MOV tmp2, 0x11DEAD11
	SBCO tmp2, CONST_PRUSHAREDRAM, PRU_RTA_SCRATCH + 10, 4
	
	LBCO tmp0, CONST_PRUSHAREDRAM, PRU_RTA_SAMPLE_MODE, 1
	QBNE WAIT_READY, tmp0.b0, RTA_CONTINUOUS
	
DIO_OPERATION:
	/*
		Write DIO
	*/
	LBCO write_mask, CONST_PRUSHAREDRAM, PRU_RTA_WRITE_MASK, 4
	QBEQ CHECK_CONTINUE, write_mask, 0
	NOT tmp0, write_mask				
	LBCO digital_out, CONST_PRUSHAREDRAM, PRU_RTA_DIGITAL_OUT, 4
	AND tmp1, write_mask, digital_out	// Set bit states for write mask
	OR r30, tmp1, tmp0					// Write bits to r30
	MOV write_mask, 0					// Prevents multiple writes of the same signal
	SBCO write_mask, CONST_PRUSHAREDRAM, PRU_RTA_WRITE_MASK, 4
	
CHECK_CONTINUE:
	LBCO tmp0, CONST_PRUSHAREDRAM, PRU_RTA_SAMPLE_MODE, 1
	QBNE WAIT_READY, tmp0.b0, RTA_CONTINUOUS
	
	QBA DIO_OPERATION