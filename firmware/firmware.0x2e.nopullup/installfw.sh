make
for f in *.dts; 
	do  file="$(echo $f | sed s/-00A0.dts//)"; 
	echo $file > /sys/devices/bone_capemgr.9/slots; 
done
cat /sys/devices/bone_capemgr.9/slots