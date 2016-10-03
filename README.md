# Masterproef
Master thesis using LWIR camera with a normal camera to improve human detection in bad weather conditions.


udev file so we don't need root permissions
	SUBSYSTEMS=="usb", ATTRS{idVendor}=="289d", ATTRS{idProduct}=="0010", MODE="0660", GROUP="input"
Don't forget to add your user to the right group
