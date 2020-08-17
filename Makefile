LDLIBS=-lmosquitto

all: js2mqtt
js2mqtt: js2mqtt.c

install: js2mqtt js2mqtt@.service 50-js2mqtt.rules
	mkdir -p /usr/local/bin
	cp js2mqtt /usr/local/bin
	# Update systemd units
	cp js2mqtt@.service /lib/systemd/system
	# Update and reload udev rules
	cp 50-js2mqtt.rules /etc/udev/rules.d/
	udevadm control -R
