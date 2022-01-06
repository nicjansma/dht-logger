//
// Copyright (c) 2022 Nic Jansma, https://nicj.net
// https://github.com/nicjansma/dht-logger-mqtt
//

//
// Built on top of other's hard work, see README.md for details
//

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

//
// Includes
//
#include <PietteTech_DHT.h>

//
// Configuration
//

// device name
#define DEVICE_NAME "your_device_name"
#define FRIENDLY_NAME "Your Device Name"

// sensor type: [DHT11, DHT22, DHT21, AM2301]
#define DHT_TYPE AM2302

// which digital pin for the DHT
#define DHT_PIN D3

// DHT timeout (milliseconds)
#define DHT_TIMEOUT 2000

// Water sensor (optional)
#define WATER_SENSOR_ENABLED 0
#define WATER_SENSOR D0

// How long to wait before noting that the alarm has switched states -- this
// helps stop a super-sensitive sensor from ping-ponging back and forth a lot.
#define WATER_SENSOR_DEBOUNCE_SECONDS 300

// which digital pin for the Photon/Spark Core/Electron LED
#define LED_PIN D7

// whether to use Farenheit instead of Celsius
#define USE_FARENHEIT 1

// min/max values (sanity checks)
#define MIN_TEMPERATURE -30
#define MAX_TEMPERATURE 120

#define MIN_HUMIDITY 0
#define MAX_HUMIDITY 100

// sensor check interval (seconds)
#define CHECK_INTERVAL 60

// AdaFruit integration
#define ADAFRUIT_ENABLED 0
#define ADAFRUIT_API_KEY "YOURAPIKEY"
#define ADAFRUIT_FEED_TEMPERATURE "temp"
#define ADAFRUIT_FEED_HUMIDITY "humidity"
#define ADAFRUIT_FEED_HEAT_INDEX "heat-index"

// ThingSpeak integration
#define THINGSPEAK_ENABLED 0
#define THINGSPEAK_CHANNEL 123456
#define THINGSPEAK_API_KEY "YOURAPIKEY"

// HTTP POST integration
#define HTTP_POST 0
#define HTTP_POST_HOST "myhost.com"
#define HTTP_POST_PORT 80
#define HTTP_POST_PATH "/"

// Particle event
#define PARTICLE_EVENT 0
#define PARTICLE_EVENT_NAME "dht-logger-log"

// MQTT server
#define MQTT_ENABLED 1
#define MQTT_SERVER "192.168.0.150"
#define MQTT_PORT 1883
#define MQTT_TOPIC "particle"
#define MQTT_KEEPALIVE_TIMEOUT 300
#define MQTT_DEVICE_MODEL "Photon"
#define MQTT_HOME_ASSISTANT_DISCOVERY "homeassistant"

//
// Integration Includes
//

// AdaFruit.io
#if ADAFRUIT_ENABLED
#include "Adafruit_IO_Client.h"
#include "Adafruit_IO_Arduino.h"
#endif

// ThingSpeak
#if THINGSPEAK_ENABLED
#include "ThingSpeak/ThingSpeak.h"
#endif

#if HTTP_POST
#include "HttpClient/HttpClient.h"
#endif

// MQTT
#if MQTT_ENABLED
#include <MQTT.h>
#endif

//
// Locals
//
TCPClient tcpClient;

PietteTech_DHT dht(DHT_PIN, DHT_TYPE);

float humidity;
float temperature;
float heatIndex;

char humidityString[10];
char temperatureString[10];
char heatIndexString[10];

#if WATER_SENSOR_ENABLED
// whether or not water is sensed: 0 = no, 1 = yes
int water = 0;

// the alarm state: 0 = off, 1 = on
int waterAlarm = 1;

// the last time we switched alarm states (ms since Unix epoch)
int waterAlarmSwitchTime = 0;
#endif

int failed = 0;

// last time since we sent sensor readings
int lastUpdate = 0;

#if ADAFRUIT_ENABLED
Adafruit_IO_Client aio = Adafruit_IO_Client(tcpClient, ADAFRUIT_API_KEY);
Adafruit_IO_Feed aioFeedTemperature = aio.getFeed(ADAFRUIT_FEED_TEMPERATURE);
Adafruit_IO_Feed aioFeedHumidity = aio.getFeed(ADAFRUIT_FEED_HUMIDITY);
Adafruit_IO_Feed aioFeedHeatIndex = aio.getFeed(ADAFRUIT_FEED_HEAT_INDEX);
#endif

#if HTTP_POST
HttpClient http;

// Headers currently need to be set at init, useful for API keys etc.
http_header_t httpHeaders[] = {
    { "Content-Type", "application/json" },
    { "Accept" , "*/*"},
    { NULL, NULL }
};

http_response_t response;
http_request_t request;
#endif

// for HTTP POST and Particle.publish payloads
char payload[1024];

// local IP address
char localIpString[24];

// MQTT client
#if MQTT_ENABLED
void mqttCallback(char* topic, byte* payload, unsigned int length);
MQTT mqttClient(MQTT_SERVER, MQTT_PORT, 1024, MQTT_KEEPALIVE_TIMEOUT, mqttCallback);
bool mqttSentDiscovery = false;
char mqttTopic[1024];
char mqttStateTopic[1024];
char mqttPayload[1024];
char mqttDevice[1024];
#endif

//
// Function Declarations
//
void checkDHT();
void checkWaterAlarm();

//
// Functions
//
/**
 * Setup
 */
void setup() {
    // LED high while doing setup
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    // configure Particle variables - float isn't accepted, so we have to use string versions
    Particle.variable("temperature", &temperatureString[0], STRING);
    Particle.variable("humidity", &humidityString[0], STRING);
    Particle.variable("heatIndex", &heatIndexString[0], STRING);

    Particle.variable("status", &failed, INT);

    // start the DHT sensor
    dht.begin();
    
    // water sensor
#if WATER_SENSOR_ENABLED
    pinMode(WATER_SENSOR, INPUT);

    // publish the water variable
    Particle.variable("water", &water, INT);

    // publish the alarm state variable
    Particle.variable("waterAlarm", &waterAlarm, INT);
#endif

#if THINGSPEAK_ENABLED
    ThingSpeak.begin(tcpClient);
#endif

#if ADAFRUIT_ENABLED
    aio.begin();
#endif

#if PARTICLE_EVENT
    // startup event
    sprintf(payload,
            "{\"device\":\"%s\",\"state\":\"starting\"}",
            DEVICE_NAME);

    Particle.publish(PARTICLE_EVENT_NAME, payload, 60, PRIVATE);
#endif

#if MQTT_ENABLED
    // MQTT state topic
    sprintf(mqttStateTopic,
            "%s/%s/state",
            MQTT_TOPIC,
            DEVICE_NAME);

    // connect to MQTT server
    mqttClient.connect(DEVICE_NAME);
    
    sprintf(mqttDevice,
        "\"device\":{\"identifiers\":[\"%s\"],\"manufacturer\":\"Particle\",\"model\":\"%s\",\"name\":\"%s\",\"sw_version\":\"1.0\"}",
        DEVICE_NAME,
        MQTT_DEVICE_MODEL,
        FRIENDLY_NAME);
#endif

    // share the IP address
    IPAddress myIp = WiFi.localIP();
    sprintf(localIpString, "%d.%d.%d.%d", myIp[0], myIp[1], myIp[2], myIp[3]);
    Particle.variable("ip", localIpString, STRING);

    // LED low once done
    digitalWrite(LED_PIN, LOW);

    // run the first measurement
    loop();
}

/**
 * Event loop
 */
void loop() {
    int now = Time.now();

#if MQTT_ENABLED
    if (!mqttClient.isConnected()) {
        // re-connect to MQTT server
        mqttClient.connect(DEVICE_NAME);
    } else {
        if (!mqttSentDiscovery) {
            //
            // Temperature
            //
            // MQTT discovery topic
            sprintf(mqttTopic,
                    "%s/sensor/%s/temperature/config",
                    MQTT_HOME_ASSISTANT_DISCOVERY,
                    DEVICE_NAME);

            // MQTT discovery payloads
            sprintf(mqttPayload,
                    "{\"unique_id\":\"%s_temperature\",\"device_class\":\"temperature\",\"name\":\"%s Temperature\",\"state_topic\":\"%s/%s/state\",\"json_attributes_topic\":\"%s/%s/state\",\"unit_of_measurement\":\"°%s\",\"value_template\":\"{{ value_json.temperature }}\",%s}",
                    DEVICE_NAME,
                    FRIENDLY_NAME,
                    MQTT_TOPIC,
                    DEVICE_NAME,
                    MQTT_TOPIC,
                    DEVICE_NAME,
                    USE_FARENHEIT ? "F" : "C",
                    mqttDevice);
                    
            mqttSend(mqttTopic, mqttPayload);

            //
            // Humidity
            //

            // MQTT discovery payloads
            sprintf(mqttTopic,
                    "%s/sensor/%s/humidity/config",
                    MQTT_HOME_ASSISTANT_DISCOVERY,
                    DEVICE_NAME);

            // MQTT discovery payloads
            sprintf(mqttPayload,
                    "{\"unique_id\":\"%s_humidity\",\"device_class\":\"humidity\",\"name\":\"%s Humidity\",\"state_topic\":\"%s/%s/state\",\"json_attributes_topic\":\"%s/%s/state\",\"unit_of_measurement\":\"%%\",\"value_template\":\"{{ value_json.humidity }}\",%s}",
                    DEVICE_NAME,
                    FRIENDLY_NAME,
                    MQTT_TOPIC,
                    DEVICE_NAME,
                    MQTT_TOPIC,
                    DEVICE_NAME,
                    mqttDevice);
                    
            mqttSend(mqttTopic, mqttPayload);

#if WATER_SENSOR_ENABLED
            //
            // Water Alarm
            //

            // MQTT discovery payloads
            sprintf(mqttTopic,
                    "%s/binary_sensor/%s/water/config",
                    MQTT_HOME_ASSISTANT_DISCOVERY,
                    DEVICE_NAME);

            // MQTT discovery payloads
            sprintf(mqttPayload,
                    "{\"unique_id\":\"%s_water\",\"device_class\":\"moisture\",\"name\":\"%s Water\",\"state_topic\":\"%s/%s/state\",\"json_attributes_topic\":\"%s/%s/state\",\"value_template\":\"{{ value_json.water }}\",\"payload_on\":\"on\",\"payload_off\":\"off\",%s}",
                    DEVICE_NAME,
                    FRIENDLY_NAME,
                    MQTT_TOPIC,
                    DEVICE_NAME,
                    MQTT_TOPIC,
                    DEVICE_NAME,
                    mqttDevice);
                    
            mqttSend(mqttTopic, mqttPayload);
#endif

            //
            // State
            //
            sprintf(mqttPayload,
                    "{\"device\":\"%s\",\"status\":\"online\"}",
                    DEVICE_NAME);

            mqttSend(mqttStateTopic, mqttPayload);

            mqttSentDiscovery = true;
        }

        mqttClient.loop();
    }
#endif

    // only run every CHECK_INTERVAL seconds
    if (now - lastUpdate < CHECK_INTERVAL) {
        return;
    }

    lastUpdate = now;

    checkWaterAlarm();
    checkDHT();
}

void checkDHT() {
    // turn on LED when updating
    digitalWrite(LED_PIN, HIGH);

    // read humidity and temperature values
    failed = dht.acquireAndWait(DHT_TIMEOUT);

    temperature = USE_FARENHEIT ? dht.getFahrenheit() : dht.getCelsius();
    humidity = dht.getHumidity();

    if (failed != 0
        || temperature == NAN
        || humidity == NAN
        || temperature > MAX_TEMPERATURE
        || temperature < MIN_TEMPERATURE
        || humidity > MAX_HUMIDITY
        || humidity < MIN_HUMIDITY) {
#if MQTT_ENABLED
        // MQTT update
        sprintf(mqttPayload,
                "{\"device\":\"%s\",\"status\":\"failed\",\"code\":%d,\"temperature_bad\":\"%.2f\",\"humidity_bad\":\"%.2f\"}",
                DEVICE_NAME,
                failed,
                temperature,
                humidity);

        mqttSend(mqttStateTopic, mqttPayload);
#endif
    } else {
        failed = 0;

        // calculate the heat index
        heatIndex = dht.getHeatIndex();

        // convert floats to strings for Particle variables
        sprintf(temperatureString, "%.2f", temperature);
        sprintf(humidityString, "%.2f", humidity);
        sprintf(heatIndexString, "%.2f", heatIndex);

#if THINGSPEAK_ENABLED
        // set all 3 fields first
        ThingSpeak.setField(1, temperatureString);
        ThingSpeak.setField(2, humidityString);
        ThingSpeak.setField(3, heatIndexString);

        // send all fields at once
        ThingSpeak.writeFields(THINGSPEAK_CHANNEL, THINGSPEAK_API_KEY);
#endif

#if ADAFRUIT_ENABLED
        aioFeedTemperature.send(temperature);
        aioFeedHumidity.send(humidity);
        aioFeedHeatIndex.send(heatIndex);
#endif

#if WATER_SENSOR_ENABLED
        sprintf(payload,
            "{\"device\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f,\"heatIndex\":%.2f,\"water\":\"%s\"}",
            DEVICE_NAME,
            temperature,
            humidity,
            heatIndex,
            water ? "on" : "off");
#else
        sprintf(payload,
            "{\"device\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f,\"heatIndex\":%.2f}",
            DEVICE_NAME,
            temperature,
            humidity,
            heatIndex);
#endif

#if HTTP_POST
        request.hostname = HTTP_POST_HOST;
        request.port = HTTP_POST_PORT;
        request.path = HTTP_POST_PATH;
        request.body = payload;

        http.post(request, response, httpHeaders);
#endif

#if PARTICLE_EVENT
        Spark.publish(PARTICLE_EVENT_NAME, payload, 60, PRIVATE);
#endif

#if MQTT_ENABLED
        mqttSend(mqttStateTopic, payload);
#endif
    }

    // done updating
    digitalWrite(LED_PIN, LOW);
}

//
// Water Alarm
//
#if WATER_SENSOR_ENABLED
void checkWaterAlarm() {
    water = isExposedToWater();

    if (water) {
        digitalWrite(LED_PIN, HIGH);

        //
        // Alarm ON
        //
        if (waterAlarm == 0) {
            // only alarm if we're past the debounce interval
            int now = Time.now();
            if (now - waterAlarmSwitchTime > WATER_SENSOR_DEBOUNCE_SECONDS) {
                waterAlarm = 1;
                waterAlarmSwitchTime = now;

#if PARTICLE_EVENT
                Particle.publish("water_alarm", "on", 60, PRIVATE);
#endif
            }
        }
    } else {
        digitalWrite(LED_PIN, LOW);

        //
        // Alarm off
        //
        if (waterAlarm == 1) {
            // only alarm if we're past the debounce interval
            int now = Time.now();
            if (now - waterAlarmSwitchTime > WATER_SENSOR_DEBOUNCE_SECONDS) {
                waterAlarm = 0;
                waterAlarmSwitchTime = now;

#if PARTICLE_EVENT
                Particle.publish("water_alarm", "off", 60, PRIVATE);
#endif
            }
        }
    }
}

// determine if we're exposed to water or not
boolean isExposedToWater() {
    if (digitalRead(WATER_SENSOR) == LOW) {
        return true;
    } else {
        return false;
    }
}
#else
void checkWaterAlarm() {}
#endif

#if MQTT_ENABLED
//
// MQTT functions
//
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // TODO
}

void mqttSend(char* topic, char* payload) {
    if (!mqttClient.isConnected()) {
        return;
    }

    mqttClient.publish(topic, payload, true);
}
#endif