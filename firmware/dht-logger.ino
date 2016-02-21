//
// Copyright (c) 2016 Nic Jansma, http://nicj.net
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
#include "DHT.h"
#include "application.h"

//
// Configuration
//

// device name
#define DEVICE_NAME "YOURDEVICENAME"

// sensor type: [DHT11, DHT22, DHT21, AM2301]
#define DHTTYPE DHT22

// which digital pin for the DHT
#define DHTPIN D3

// which digital pin for the Photon/Spark Core/Electron LED
#define LEDPIN D7

// whether to use Farenheit instead of Celsius
#define USE_FARENHEIT 1

// min/max values (sanity checks)
#define MIN_TEMPERATURE -30
#define MAX_TEMPERATURE 120

#define MIN_HUMIDITY 0
#define MAX_HUMIDITY 100

// sensor sending interval (seconds)
#define SEND_INTERVAL 10

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

//
// Locals
//
TCPClient tcpClient;

DHT dht(DHTPIN, DHTTYPE);

float humidity;
float temperature;
float heatIndex;

char humidityString[10];
char temperatureString[10];
char heatIndexString[10];

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

/**
 * Setup
 */
void setup() {
    pinMode(LEDPIN, OUTPUT);
    digitalWrite(LEDPIN, HIGH);

    // configure Particle variables - float isn't accepted, so we have to use string versions
    Particle.variable("temperature", &temperatureString[0], STRING);
    Particle.variable("humidity", &humidityString[0], STRING);
    Particle.variable("heatIndex", &heatIndexString[0], STRING);

    Particle.variable("status", &failed, INT);

    // start the DHT sensor
    dht.begin();

#if THINGSPEAK_ENABLED
    ThingSpeak.begin(tcpClient);
#endif

#if ADAFRUIT_ENABLED
    aio.begin();
#endif
}

/**
 * Event loop
 */
void loop() {
    int now = Time.now();

    // only run every SEND_INTERVAL seconds
    if (now - lastUpdate < SEND_INTERVAL) {
        return;
    }

    // turn on LED when updating
    digitalWrite(LEDPIN, HIGH);

    lastUpdate = now;

    failed = 0;

    // read humidity and temperature values
    humidity = dht.readHumidity();
    temperature = dht.readTemperature(USE_FARENHEIT);

    if (temperature == NAN
        || humidity == NAN
        || temperature > MAX_TEMPERATURE
        || temperature < MIN_TEMPERATURE
        || humidity > MAX_HUMIDITY
        || humidity < MIN_HUMIDITY) {
        // if any sensor failed, bail on updates
        failed = 1;
    } else {
        failed = 0;

        // calculate the heat index
        heatIndex = dht.computeHeatIndex(temperature, humidity, USE_FARENHEIT);

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

        sprintf(payload,
            "{\"device\":\"%s\",\"temperature\":%.2f,\"humidity\":%.2f,\"heatIndex\":%.2f}",
            DEVICE_NAME,
            temperature,
            humidity,
            heatIndex);

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
    }

    // done updating
    digitalWrite(LEDPIN, LOW);
}