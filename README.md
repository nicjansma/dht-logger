# Particle (Spark) Core / Photon / Electron Remote Temperature and Humidity Logger (and optional Water Alarm)

By [Nic Jansma](http://nicj.net)

This is a remote temperature, humidity and water alarm that logs data to a number of optional services, including:

* [Adafruit.io](https://io.adafruit.com)
* [ThingSpeak](https://thingspeak.com/)
* [DynamoDB](https://aws.amazon.com/dynamodb/)
* Any HTTP endpoint
* MQTT

I am currently using this on my kegerator (keezer) to monitor its temperature:

![AdaFruit.io](https://github.com/nicjansma/dht-logger/raw/master/images/keezer.jpg)

## Hardware

For hardware, I'm using a [Particle Photon](https://store.particle.io/), a $19 Arduino-compatible development board with built-in WiFi.  It's hooked up to a [AM2303 (DHT22)](https://www.adafruit.com/products/393) temperature and humidity sensor.  The Photon can be wrapped in a [case](https://www.thingiverse.com/thing:413025) to protect it.

**Build list**:

* Arduino compatible board, such as:
  * [Particle Photon](https://store.particle.io/) (WiFi): $19.00
  * [Particle Electron](https://store.particle.io/) (GSM): $59.00
* Temperature + Humidity Sensor, such as:
  * [AM2302 (wired DHT22)](https://www.adafruit.com/products/393): $15.00 @ Adafruit
  * [DHT22](https://www.adafruit.com/products/385): $9.95 @ Adafruit
  * [DHT11](https://www.adafruit.com/products/386): $5.00 @ Adafruit

Total build cost: $24.00 - $74.00

In my case, a AM2302 is hooked up to the Photon as such:

* Red (power) to VIN
* Black (ground) to GND
* Yellow (data) to D3

![AdaFruit.io](https://github.com/nicjansma/dht-logger/raw/master/images/photon.jpg)

My Photon is then wrapped in a [case](https://www.thingiverse.com/thing:413025) and taped to my keezer:

![AdaFruit.io](https://github.com/nicjansma/dht-logger/raw/master/images/case.jpg)

## Firmware

The `firmware/` directory contains all of the code you will need for a Particle device.

The main code is in `dht-logger.ino`.  You will also need to add the `PietteTech_DHT` library (in the IDE), which is a library that helps decode the temperature and humidity data from the sensor.

If you want to log to [Adafruit.io](https://io.adafruit.com), you will also need the `Adafruit_IO_Particle` library.

If you want to use MQTT, you will also need the `MQTT` library.

You can paste the contents `dht-logger.ino` into the [Particle Web IDE](https://build.particle.io) interface:

![Particle Web IDE](https://github.com/nicjansma/dht-logger/raw/master/images/particle-build.png)

The firmware has several configuration options, all in `dht-logger.ino`:

* `DEVICE_NAME`: A name for this device, used when logging to DynamoDB and the HTTP end-points.  It's often desirable to have this be all lower-case letters, and spaces replaced with `-` or `_`, depending on the endpoints.
* `FRIENDLY_NAME`: A friendly name for this device, for display.  Can include spaces.
* `DHT_TYPE`: Which sensor type, such as `DHT11`, `DHT22`, `DHT21`, `AM2301`, `AM2302`
* `DHT_PIN`: Which digital pin the DHT is connected to
* `DHT_TIMEOUT`: How long the DHT sensor be polled for
* `LED_PIN`: Which pin has a LED (which blinks each time a reading is being taken)
* `USE_FARENHEIT`: Whether to use Farenheit versus Celsius
* `MIN_TEMPERATURE`, `MAX_TEMPERATURE`, `MIN_HUMIDITY` and `MAX_HUMIDITY`: I've found that my DHT22 sensor sometimes gives wildly inaccurate readings (such as -300*F).  These min/max values help weed out incorrect readings.
* `CHECK_INTERVAL`: How often to check data and send to the logging services
* `ADA_FRUIT_ENABLED`: Whether or not to send data to [Adafruit.io](http://io.adafruit.com)
  * `ADAFRUIT_API_KEY`: Your Adafruit.io API key
  * `ADAFRUIT_FEED_*`: Which Adafruit.io feed to use for each data-point
* `THINGSPEAK_ENABLED`: Whether or not to send data to [ThingSpeak](https://thingspeak.com/)
  * `THINGSPEAK_CHANNEL`: Which channel to log to
  * `THINGSPEAK_API_KEY`: ThingSpeak API key
* `HTTP_POST`: Whether or not to send data to a HTTP endpoint
  * `HTTP_POST_HOST`: Host to send to
  * `HTTP_POST_PORT`: Port to send to
  * `HTTP_POST_PATH`: Path to send to
* `PARTICLE_EVENT`: Whether or not to send data via a [Particle event](https://docs.particle.io/reference/api/#events)
  * `PARTICLE_EVENT_NAME`: Which event name to use
* `MQTT_ENABLED`: Whether MQTT is enabled or not
    * `MQTT_SERVER`: MQTT server
    * `MQTT_PORT`: MQTT port
    * `MQTT_TOPIC`: MQTT root state topic
    * `MQTT_KEEPALIVE_TIMEOUT`: MQTT keepalive timeout
    * `MQTT_DEVICE_MODEL` MQTT Device model
    * `MQTT_HOME_ASSISTANT_DISCOVERY: MQTT topic for discovery for Home Assistant

Many of these options are explained more in the Logging section below.

## Logging

This firmware supports sending log data natively to several services.

It currently logs:

* Temperature (in Fahrenheit or Celsius)
* [Relative Humidity](https://en.wikipedia.org/wiki/Relative_humidity) (percentage)
* [Heat Index](https://en.wikipedia.org/wiki/Heat_index)

### Adafruit.io

[Adafruit.io](https://io.adafruit.com) provides an easy way to log your IoT data, and has a nice dashboard interface for visualizing it:

![AdaFruit.io](https://github.com/nicjansma/dht-logger/raw/master/images/adafruit.png)

To use Adafruit.io, you will need to create 3 feeds, for temperature, humidity and heat index.  Put these feed names into the `ADAFRUIT_FEED_*` defines in `dht-logger.io`.

Your API Key can be found via the _Your secret AIO Key_ button.  It should go into `ADAFRUIT_API_KEY`.

### ThingSpeak

[ThingSpeak](http://thingspeak.com) also has an easy interface for logging your IoT data, and their dashboards let you visualize it:

![AdaFruit.io](https://github.com/nicjansma/dht-logger/raw/master/images/thingspeak.png)

You will need to create a single Channel.  In that channel, you should use 3 Fields for temperature, humidity and heatindex (in that order).  This Channel ID should go into `THINGSPEAK_CHANNEL`.

Your API Key can be found under _API Keys_.  It should go into `THINGSPEAK_API_KEY`.

### DynamoDB

[DynamoDB](https://aws.amazon.com/dynamodb/) is a [NoSQL](https://aws.amazon.com/nosql/) database from Amazon Web Services, and is a great place to log your IoT data.  DynamoDB does not provide any visualizations like Adafruit or ThingSpeak, but once you have the data logged to DynamoDB, you can do whatever you want with it.  It provides a great long-term storage option for your IoT data.  DynamoDB is [free](https://aws.amazon.com/dynamodb/pricing/) for your first 25 GB.

Here's the DynamoDB dashboard showing my logged temperature data:

![AdaFruit.io](https://github.com/nicjansma/dht-logger/raw/master/images/dynamodb.png)

Here's the challenge: it's actually somewhat inconvenient to get data from a Photon into DynamoDB, primarily because the Photon does not (yet) have SSL/TLS libraries.

Both [Amazon IoT](https://aws.amazon.com/iot/) and/or [Amazon API Gateway](https://aws.amazon.com/api-gateway/) would be useful, but they only offer SSL endpoints.  So we're going to need to have a bridge that can take data from the Photon and post it to a SSL endpoint for us.  Luckily, [Particle Webhooks](https://docs.particle.io/guide/tools-and-features/webhooks/) can do this for us.

Here's how we're going to get all of these services working for us to be able to log to DynamoDB:

1. Create a Particle Webhook that listens for an event with our data
2. Have the Particle Webhook POST this data to a Amazon API Gateway endpoint
3. Have the Amazon API Gateway run an [Amazon Lambda](https://aws.amazon.com/lambda/) function
4. Have the Lambda function create rows in our DynamoDB table

It sounds a bit complicated, but the configuration for this should be relatively straightforward.

#### Amazon DynamoDB

First, you'll need to create a DynamoDB table to log your data.  You can do this via the [Amazon AWS Console](https://console.aws.amazon.com):

1. Open the [Amazon AWS Console](https://console.aws.amazon.com)
2. Select DynamoDB from the list of services
3. Click on _Tables_ and click _Create table_
4. _The Partition key_ should be `device`, a String, and _Sort key_ should be `time`, a Number
5. You can _Use default settings_ if you wish.  No other indexes are required.  I reduced my read/write capacity units to 1/1, since I only have a single device logging to this table once every 10 seconds.  You get [up to 25 GB and 25 read/write capacity](https://aws.amazon.com/free/) free as part of the AWS Free Tier.

![AdaFruit.io](https://github.com/nicjansma/dht-logger/raw/master/images/dynamodb-setup.png)

Your DynamoDB table is now configured!

#### Amazon Lambda

[Amazon Lambda](https://aws.amazon.com/lambda/) is a handy service from Amazon that lets you run small code snippets in the cloud when triggered by various events.  We're going to create an [Amazon API Gateway](https://aws.amazon.com/api-gateway/) endpoint next that triggers our function.  The Lambda function will be responsible for adding our data into the DynamoDB database.  Amazon Lambda is [free](https://aws.amazon.com/free/) for the first 1 million requests per month.

Here are the steps required to create a Lambda function:

1. Open the [Amazon AWS Console](https://console.aws.amazon.com)
2. Select Lambda from the list of services
3. Click _Create a Lambda function_
4. You can _Skip_ the blueprint
5. _Name_ your Lambda function something like `iot-dynamodb`, and select Node.js as the _Runtime_
6. For the code, paste in the code from `lambda.js`
7. Edit the `tableName` variable to be the DynamoDB table name you selected
8. Leave _Handler_ as _index.handler_ and change _Role_ to _Basic with DynamoDB_.  This will popup a new window for you to create a new _IAM Role_.
9. You can probably leave the _Advanced setting_ as their defaults, with 128 _Memory (MB)_ and a 3 second _Timeout_
10. Hit _Create_ and you're all set

![AdaFruit.io](https://github.com/nicjansma/dht-logger/raw/master/images/lambda-setup.png)

#### Amazon API Gateway

Next, we're going to configure an [Amazon API Gateway](https://aws.amazon.com/api-gateway/) endpoint.  The API Gateway endpoint gives us a convenient web URL, that when called, will run our Lambda function.  The Lambda function interface gives you a convenient way to do this as well.

Here's the steps to setup the API Gateway:

1. Open the Lambda function we just created
2. Click on _API endpoints_
3. Click _Add API endpoint_
4. For _API endpoint type_, select _API Gateway_
5. You can use whatever name for the API you want, but it defaults to _LambdaMicroservice_
6. The _Resource name_ can be whatever you want, and defaults to the name of the Lambda function
7. Change _Method_ to _POST_
8. Change _Security_ to _Open with access key_ (so we can secure the endpoint with a secret key)
9. Click _Submit_

![AdaFruit.io](https://github.com/nicjansma/dht-logger/raw/master/images/api-gateway-setup-1.png)

Note the new _API endpoint URL_ for later use:

![AdaFruit.io](https://github.com/nicjansma/dht-logger/raw/master/images/api-gateway-setup-2.png)

1. Next, switch to the _Amazon API Gateway_ service so we can get an API key
2. Change to the _API Keys_ dropdown
3. Click on _Create API Key_
4. _Name_ it whatever, you want, and select _Enabled_, and click _Create_
5. Change _Select API_ to _LambdaMicroservice_, and _Stage_ to _prod_
6. Click _Add_
7. Note your new _API key_ for later
8. Click _Save_

![AdaFruit.io](https://github.com/nicjansma/dht-logger/raw/master/images/api-gateway-setup-3.png)

Phew!  We now have an Amazon Lambda function that will create rows in our Amazon DynamoDB table by hitting an Amazon API Gateway endpoint URL.

#### Particle Webhook

A [Particle Webhook](https://docs.particle.io/guide/tools-and-features/webhooks/) will be the final bridge that gathers data from the Particle and sends it to an SSL endpoint (which the Photon doesn't yet natively support).

A Webhook is a simple command that instructs Particle to send data to another place when a Particle event occurs.

1. First, configure the `PARTICLE_EVENT_NAME` in your `dht-logger.ino` to be whatever you wish, such as `dht-logger-log`.
   
2. Ensure `PARTICLE_EVENT` is `1` in the same file
   
3. Next, [install the `particle-cli` NodeJS module](https://github.com/spark/particle-cli):
   
   `npm install -g particle-cli`
   
4. Copy `particle-webhook.json` into a local file
   
5. Edit `particle-webhook.json` and replace `YOURENDPOINT` with the Amazon API Gateway endpoint and `YOURAPIKEY` with the Amazon API Gateway API key you created
   
6. Run this command to [install the webhook](https://docs.particle.io/guide/tools-and-features/webhooks/#cli-command-reference):
   
   `particle webhook create particle-webhook.json`

That should be it!

Now, your Particle will emit an event that triggers a webhook that hits an API Gateway that runs a Lambda that inserts a row into DynamoDB.

### HTTP POST

The device can be configured to log to any HTTP POST URL.

Note that HTTPS is not supported (yet) by the Photon.

To configure the HTTP POST, modify the `HTTP_POST` and `HTTP_POST_*` variables in `dht-logger.ino`.

The payload for the HTTP POST will be the same as the DynamoDB data:

```json
{
  "device": "[device name]",
  "temperature": 72.4,
  "humidity": 50.0,
  "heatIndex": 74.6
}
```

### MQTT

The device can be configured to send [MQTT discovery](https://www.home-assistant.io/docs/mqtt/discovery/) and [state](https://www.home-assistant.io/docs/mqtt/) payloads for [Home Assistant](https://www.home-assistant.io/).

To configure, set the `MQTT_*` variables.

![AdaFruit.io](https://github.com/nicjansma/dht-logger/raw/master/images/home-assistant.png)

## SmartThings

I also have this device monitored via my [Samsung SmartThings](https://www.smartthings.com/) app:

![AdaFruit.io](https://github.com/nicjansma/dht-logger/raw/master/images/smartthings.png)

To configure this, you'll want to use the Device Handler in my [smart-things](https://github.com/nicjansma/smart-things/tree/master/devicetypes/nicjansma/particle-temperature-humidity-logger) project.  You can install it via the [SmartThings API Console](https://graph.api.smartthings.com).

## Thanks

This project was built on top of a lot of work done by others, including:

* https://snowulf.com/2015/08/05/tutorial-aws-api-gateway-to-lambda-to-dynamodb/
* https://gist.github.com/wgbartley/8301123
* https://github.com/adafruit/DHT-sensor-library/blob/master/DHT.h
* https://github.com/adafruit/Adafruit_IO_Arduino

## Version History

* 2016-02-20
  * Initial version
* 2022-01-05
  * Switched to using official libraries (`PietteTech_DHT` for DHT and `Adafruit_IO_Particle` if needed)
  * Added MQTT support
  * Added optional Water Sensor
* 2026-02-28
  * Stability fixes
  * Serial console logging
