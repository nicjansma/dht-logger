//
// Based on https://gist.github.com/wgbartley/8301123
// and https://github.com/adafruit/DHT-sensor-library/blob/master/DHT.cpp
//

#include "DHT.h"
#include "math.h"

/**
 * Constructor
 */
DHT::DHT(uint8_t pin, uint8_t type, uint8_t count) {
    _pin = pin;
    _type = type;
    _count = count;
    _firstReading = true;
}

/**
 * Sets up the pins
 */
void DHT::begin(void) {
    // set up the pins!
    pinMode(_pin, INPUT);

    digitalWrite(_pin, HIGH);

    _lastReadTime = 0;
}

/**
 * Gets the current temperature
 */
float DHT::readTemperature(bool farenheit) {
    float temp = NAN;

    if (read()) {
        switch (_type) {
            case DHT11:
                temp = _data[2];

            case DHT22:
            case DHT21:
                temp = _data[2] & 0x7F;
                temp *= 256;
                temp += _data[3];
                temp /= 10;

                // negative temp
                if (_data[2] & 0x80) {
                    temp *= -1;
                }
        }

        if (farenheit) {
            temp = convertCtoF(temp);
        }
    }

    return temp;
}

/**
 * Converts Celsius to Farenheit
 */
float DHT::convertCtoF(float c) {
    return c * 9 / 5 + 32;
}

/**
 * Converts Farenheit to Celsius
 */
float DHT::convertFtoC(float f) {
  return (f - 32) * 0.55555;
}

/**
 * Gets the current humidity
 */
float DHT::readHumidity(void) {
    float humidity = NAN;

    if (read()) {
        switch (_type) {
            case DHT11:
                humidity = _data[0];

            case DHT22:
            case DHT21:
                humidity = _data[0];
                humidity *= 256;
                humidity += _data[1];
                humidity /= 10;

                return humidity;
        }
    }

    return humidity;
}

/**
 * Computes the Heat Index
 */
float DHT::computeHeatIndex(float temperature, float percentHumidity, bool isFahrenheit) {
    // Using both Rothfusz and Steadman's equations
    // http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml
    float hi;

    if (!isFahrenheit) {
        temperature = convertCtoF(temperature);
    }

    hi = 0.5 * (temperature + 61.0 + ((temperature - 68.0) * 1.2) + (percentHumidity * 0.094));

    if (hi > 79) {
        hi = -42.379 +
             2.04901523 * temperature +
            10.14333127 * percentHumidity +
            -0.22475541 * temperature*percentHumidity +
            -0.00683783 * pow(temperature, 2) +
            -0.05481717 * pow(percentHumidity, 2) +
             0.00122874 * pow(temperature, 2) * percentHumidity +
             0.00085282 * temperature*pow(percentHumidity, 2) +
            -0.00000199 * pow(temperature, 2) * pow(percentHumidity, 2);
    }

    if ((percentHumidity < 13) && (temperature >= 80.0) && (temperature <= 112.0)) {
        hi -= ((13.0 - percentHumidity) * 0.25) * sqrt((17.0 - abs(temperature - 95.0)) * 0.05882);
    } else if ((percentHumidity > 85.0) && (temperature >= 80.0) && (temperature <= 87.0)) {
        hi += ((percentHumidity - 85.0) * 0.1) * ((87.0 - temperature) * 0.2);
    }

    return isFahrenheit ? hi : convertFtoC(hi);
}

/**
 * Gets a reading from the DHT
 */
bool DHT::read(void) {
    uint8_t lastState = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i = 0;
    unsigned long currentTime;

    // pull the pin high and wait 250 milliseconds
    digitalWrite(_pin, HIGH);
    delay(250);

    currentTime = millis();
    if (currentTime < _lastReadTime) {
        // ie there was a rollover
        _lastReadTime = 0;
    }

    if (!_firstReading && ((currentTime - _lastReadTime) < 2000)) {
        //delay(2000 - (currentTime - _lastReadTime));
        return true; // return last correct measurement
    }

    _firstReading = false;
    Serial.print("Currtime: "); Serial.print(currentTime);
    Serial.print(" Lasttime: "); Serial.print(_lastReadTime);
    _lastReadTime = millis();

    // zero-out the data
    _data[0] = _data[1] = _data[2] = _data[3] = _data[4] = 0;

    // now pull it low for ~20 milliseconds
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    delay(20);
    noInterrupts();
    digitalWrite(_pin, HIGH);
    delayMicroseconds(40);
    pinMode(_pin, INPUT);

    // read in timings
    for (i = 0; i < MAXTIMINGS; i++) {
        counter = 0;

        while (digitalRead(_pin) == lastState) {
            counter++;
            delayMicroseconds(1);

            if (counter == 255) {
                break;
            }
        }

        lastState = digitalRead(_pin);

        if (counter == 255) {
            break;

        }

        // ignore first 3 transitions
        if ((i >= 4) && (i%2 == 0)) {
            // shove each bit into the storage bytes
            _data[j/8] <<= 1;

            if (counter > _count) {
                _data[j/8] |= 1;
            }

            j++;
        }
    }

    interrupts();

    // check we read 40 bits and that the checksum matches
    if ((j >= 40) && (_data[4] == ((_data[0] + _data[1] + _data[2] + _data[3]) & 0xFF))) {
        return true;
    }

    return false;
}