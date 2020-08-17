LDLIBS=-lmosquitto

all: js2mqtt
js2mqtt: js2mqtt.c

install: js2mqtt
	mkdir -p /usr/local/bin
	cp js2mqtt /usr/local/bin
	systemctl stop js2mqtt@joystick.service
	systemctl stop js2mqtt@pedals.service
	cp js2mqtt@.service /lib/systemd/system
	cp 50-js2mqtt.rules /etc/udev/rules.d
