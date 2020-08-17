LDLIBS=-lmosquitto

all: js2mqtt
js2mqtt: js2mqtt.c

install: js2mqtt
	mkdir -p /usr/local/bin
	cp js2mqtt /usr/local/bin
	systemctl stop js2mqtt.service
	cp js2mqtt.service /lib/systemd/system
	systemctl enable js2mqtt.service
	systemctl start js2mqtt.service
