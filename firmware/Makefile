all: PRU1AIO-GPIO-IN-00A0.dtbo PRU1AIO-GPIO-OUT-00A0.dtbo

PRU1AIO-GPIO-IN-00A0.dtbo:
	dtc -O dtb -o /lib/firmware/PRU1AIO-GPIO-IN-00A0.dtbo -b 0 -@ PRU1AIO-GPIO-IN.dts
	dtc -O dtb -o PRU1AIO-GPIO-IN-00A0.dtbo -b 0 -@ PRU1AIO-GPIO-IN.dts
	
PRU1AIO-GPIO-OUT-00A0.dtbo:
	dtc -O dtb -o /lib/firmware/PRU1AIO-GPIO-OUT-00A0.dtbo -b 0 -@ PRU1AIO-GPIO-OUT.dts
	dtc -O dtb -o PRU1AIO-GPIO-OUT-00A0.dtbo -b 0 -@ PRU1AIO-GPIO-OUT.dts