# Masterproef
Master thesis using LWIR camera with a normal camera to improve human detection in bad weather conditions.

###No root priv needed
Add this line to the udev file /etc/lib/udev/rules.d/50-SeekThermal.rules. Add your user to the group input. Restart your pc.
```
	SUBSYSTEMS=="usb", ATTRS{idVendor}=="289d", ATTRS{idProduct}=="0010", MODE="0660", GROUP="input"
```

