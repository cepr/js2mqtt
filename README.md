# js2mqtt
Publish Linux Joystick events to a MQTT broker

# Building
On Ubuntu:

    sudo apt install build-essential libmosquitto-dev
    make

You can also automatically install the binary and the udev and systemd template service with this command, but I would recommend to customize `Makefile` and `50-js2mqtt.rules` your needs first:

    sudo make install

# Command line usage
The complete command-line usage help is available by typing

    js2mqtt -h

# Integration with `udev` and `systemd`
The file `js2mqtt@.service` is a template systemd service. To start monitoring `/dev/input/js0`, you can type:

    sudo systemctl start js2mqtt@js0.service

You can replace `js0` by any other device name present in `/dev/input`.

Once started, the joystick events will be sent to a local Mosquitto MQTT server on the topic `/js2mqtt/js0`.

To automatically boot the service at boot, you need to enable it:

    sudo systemctl enable js2mqtt@js0.service

If you need to handle multiple joysticks, you will need to use a custom *udev* rule to create symbolic links to help differentiate the different js0, js1, etc. The file `50-js2mqtt.rules` is provided as an example:

    ACTION=="add", SUBSYSTEM=="input", KERNEL=="js*", ATTRS{name}=="Microsoft X-Box One Elite pad", SYMLINK+="input/xboxpad", TAG+="systemd", ENV{SYSTEMD_WANTS}="js2mqtt@xboxpad.service"

When a *Microsoft X-Box One Elite pad* joystick is plugged, *udev* automatically create a device name called `/dev/input/xboxpad`. This rule also instructs *systemd* to start the service `js2mqtt@xboxpad.service`. In that scenario, you don't need to enable the service with `systemctl enable`.

# License
Copyright 2020 Cedric Priscal

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
