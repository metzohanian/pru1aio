# pru1aio
BeagleBone Black PRU 1 Data Capture Library

This is intended to be a library to implement continuous analog and digital capture via the PRU (PRU1, specifically) for the BeagleBone Black.

The code is largely a rewrite of code found on Google Groups (almost entirely rewritten, actually), but shares some concepts. It is also a partial re-implementation of the C# National Instruments DAQmx library (namely, the asynchronous callback).

## Features:
 - Capture continuously on up to 7 analog channels and 16 digital channels at up to 10Ksps
 - Up to 31 bits of digital out (via r30)
 - Low skew, the trigger for capturing data is based off of the IEP timer, rather than spinlocks; this means you can program to capture data for long periods of time (hours, days, or months) having almost zero long-term skew.  There is a small amount of jitter in samples, based on specific logic flow, but the jitter is on the order of several picoseconds, and no more than ~2 microseconds.  
 - The interchannel skew of the analog inputs is configurable inversely with fidelity via the sample_average and sample_soc parameters (fidelity and skew are inversely related in the PRU).
 - Digital input has no programmatic skew.
 - Up to 20 per-channel conditional signals triggers based on buffered data (>, >=, <, <=, digital/rising edge, digital/falling edge).
 - Configurable buffer size/callback rate, double-buffered.
 - Ring buffer support.
 - Missed buffer errors.
 - Start/Pause/Stop/Reset the firmware.

It is currently a work-in-progress, and not ready for use to speak of.

## Progress:

 - Conversion to a proper library (.so) [Not Started]
 - Call-back architecture to support asyncrhonous, continuous, frame-based processing of AIO and DIO data [Complete]
 - Library-based conditional checking of data [Complete]
 - Sample use code [Complete(ish)]
 - Low-skew, fully programmable capture rate in Samples/Second [Complete]
 - Programmable fidelity for AIO [Complete]
 - Programmable AI channel profile [Complete]
 - Programmable DIOs (up to 16 In/31 Out) [In Progress]
 - Ring Buffer [In Progress]
 - C# wrapper [Not Started]
 - Rust wrapper [Not Started]
 - Proper documentation [Not Started]
 - Start/Pause/Stop/Reset the firmware. [Complete ?]
