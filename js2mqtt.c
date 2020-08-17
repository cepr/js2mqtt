/*
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
*/

#include <sys/types.h> // open()
#include <sys/stat.h> // open()
#include <fcntl.h> // open()
#include <unistd.h> // getopt(), read()
#include <string.h> // strdup()
#include <stdlib.h> // strtol(), exit()
#include <stdio.h> // fprintf(), perror(), snprintf()
#include <libgen.h> // basename()
#include <linux/joystick.h>
#include <stdbool.h> // bool, true, false
#include <mosquitto.h>

#ifndef VERSION
#define VERSION "0.1"
#endif

// Define program arguments with default values
const char* prog_name = "js2mqtt";
const char* device_path = "/dev/input/js0";
const char* mqtt_server_address = "localhost";
unsigned short mqtt_server_port = 1883;
const char* topic = "/joystick";
bool debug = false;

// Constants
#define MOSQUITTO_KEEP_ALIVE 60

static void help()
{
    fprintf(
        stderr,
        "Usage: %s [OPTION]...\n"
        "Listen for joystick events and forward them to a MQTT server.\n"
        "\n"
        "  -i DEVICE_PATH          dath to the joystick device. Default: %s\n"
        "  -o MQTT_SERVER_ADDRESS  MQTT server address. Default: %s\n"
        "  -p MQTT_SERVER_PORT     MQTT server port. Default: %hu\n"
        "  -t MQTT_TOPIC           MQTT topic. Default: %s\n"
        "  -d                      display the JSON object on the standard output\n"
        "  -v                      display version and exit\n"
        "  -h                      display this help and exit\n"
        "\n"
        "This program listens for Linux joystick events as described here:\n"
        "https://www.kernel.org/doc/Documentation/input/joystick-api.txt\n"
        "\n"
        "It then decodes those events into the following JSON format:\n"
        "{\n"
        "    \"time\": <event timestamp in milliseconds>,\n"
        "    \"value\": <value between >,\n"
        "    \"type\": <\"button\"|\"axis\">,\n"
        "    \"number\": <axis or button number>\n"
        "}\n"
        "\n"
        "Copyright 2020 Cedric Priscal\n"
        "https://github.com/cepr/js2mqtt\n"
        "\n",
        prog_name,
        device_path,
        mqtt_server_address,
        mqtt_server_port,
        topic
        );
}

static void version()
{
    fprintf(
        stderr,
        "%s " VERSION "\n"
        "Copyright 2020 Cedric Priscal\n"
        "https://github.com/cepr/js2mqtt\n"
        "\n"
        "   Licensed under the Apache License, Version 2.0 (the \"License\");\n"
        "   you may not use this file except in compliance with the License.\n"
        "   You may obtain a copy of the License at\n"
        "\n"
        "       http://www.apache.org/licenses/LICENSE-2.0\n"
        "\n"
        "   Unless required by applicable law or agreed to in writing, software\n"
        "   distributed under the License is distributed on an \"AS IS\" BASIS,\n"
        "   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
        "   See the License for the specific language governing permissions and\n"
        "   limitations under the License.\n"
        "\n",
        prog_name
    );
}

static void mosquitto_assert(int connack_code, const char* msg) {
    if (connack_code != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "%s: %s\n", msg, mosquitto_connack_string(connack_code));
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    // Extract program name
    if (argc > 0) {
        prog_name = strdup(basename(argv[0]));
    }

    // Parse command line
    int opt;
    while((opt = getopt(argc, argv, "hvdi:o:p:t:")) != -1) {
        switch(opt) {
            case 'h':
            {
                help();
                exit(EXIT_SUCCESS);
                break;
            }

            case 'v':
            {
                version();
                exit(EXIT_SUCCESS);
                break;
            }

            case 'd':
            {
                debug = true;
                break;
            }

            case 'i':
            {
                device_path = strdup(optarg);
                break;
            }

            case 'o':
            {
                mqtt_server_address = strdup(optarg);
                break;
            }

            case 'p':
            {
                char* endptr = NULL;
                mqtt_server_port = (unsigned short)strtol(optarg, &endptr, 10);
                if (endptr == NULL || *endptr != '\0') {
                    fprintf(stderr, "Invalid port specified: %s\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
            }

            case 't':
            {
                topic = strdup(optarg);
                break;
            }

            default:
            {
                // getopt already displays a warning
                exit(EXIT_FAILURE);
                break;
            }
        }
    }

    fprintf(
        stderr,
        "%s: publishing events from %s to %s:%hu...\n",
        prog_name, device_path, mqtt_server_address, mqtt_server_port
    );

    // See https://www.kernel.org/doc/Documentation/input/joystick-api.txt
    int fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        perror("Cannot open joystick device");
        exit(EXIT_FAILURE);
    }

    // Connect to MQTT
    mosquitto_lib_init();
    struct mosquitto * mosq = mosquitto_new(NULL, true, NULL);
    if (mosq == NULL) {
        perror("mosquitto_new()");
        exit(EXIT_FAILURE);
    }
    mosquitto_assert(mosquitto_connect_async(mosq, mqtt_server_address, mqtt_server_port, MOSQUITTO_KEEP_ALIVE), "mosquitto_connect_async()");
    mosquitto_assert(mosquitto_loop_start(mosq), "mosquitto_loop_start()");

    // Read loop
    while(true) {
        struct js_event e;
	    ssize_t ret = read (fd, &e, sizeof(e));
        if (ret < 0) {
            perror("Cannot read joystick events");
            exit(EXIT_FAILURE);
        } else if (ret != sizeof(e)) {
            fprintf(stderr, "Cannot read joystick events\n");
            exit(EXIT_FAILURE);
        }

        // Ignore if neither a button nor an axis event
        if (e.type & (JS_EVENT_BUTTON | JS_EVENT_AXIS) == 0) {
            continue;
        }

        // Decode in json
        char payload[256];
        size_t payloadlen = snprintf(
            payload,
            sizeof(payload),
            "{\"time\":%u,\"value\":%hd,\"type\":\"%s\",\"number\":%hhu}",
            e.time,
            e.value,
            e.type & JS_EVENT_BUTTON ? "button" : "axis",
            e.number
        );

        // Display on stdout        
        if (debug) {
            printf("%.*s\n", (int)payloadlen, payload);
        }

        // Publish to MQTT
        mosquitto_assert(mosquitto_publish(
            mosq,
		    NULL,
	        topic,
		    payloadlen,
	        payload,
		    0,
		    true),
            "mosquitto_publish()");
    }
}
