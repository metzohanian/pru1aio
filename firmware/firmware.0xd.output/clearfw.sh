cat /sys/devices/bone_capemgr.9/slots | grep bspm_P | grep _d | while read line; do 
	id="$( cut -d ':' -f 1 <<< "$line" )"; 
	echo -$id > /sys/devices/bone_capemgr.9/slots; 
done;
cat /sys/devices/bone_capemgr.9/slots