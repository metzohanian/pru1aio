all: PRU1AIO-GPIO-IN-00A0.dtbo PRU1AIO-GPIO-OUT-00A0.dtbo firmware

PRU1AIO-GPIO-IN-00A0.dtbo:
	dtc -O dtb -o /lib/firmware/PRU1AIO-GPIO-IN-00A0.dtbo -b 0 -@ PRU1AIO-GPIO-IN.dts
	dtc -O dtb -o PRU1AIO-GPIO-IN-00A0.dtbo -b 0 -@ PRU1AIO-GPIO-IN.dts
	
PRU1AIO-GPIO-OUT-00A0.dtbo:
	dtc -O dtb -o /lib/firmware/PRU1AIO-GPIO-OUT-00A0.dtbo -b 0 -@ PRU1AIO-GPIO-OUT.dts
	dtc -O dtb -o PRU1AIO-GPIO-OUT-00A0.dtbo -b 0 -@ PRU1AIO-GPIO-OUT.dts
	
FW_DIR1 = firmware.0x26.pulldown
FW_DIR2 = firmware.0x2e.nopullup
FW_DIR3 = firmware.0x5.output
FW_DIR4 = firmware.0xd.output
.PHONY: firmware

firmware:
	$(MAKE) -C $(FW_DIR1)
	$(MAKE) -C $(FW_DIR2)
	$(MAKE) -C $(FW_DIR3)
	$(MAKE) -C $(FW_DIR4)