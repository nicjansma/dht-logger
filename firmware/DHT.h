//
// Based on https://gist.github.com/wgbartley/8301123
// and https://github.com/adafruit/DHT-sensor-library/blob/master/DHT.h
//
#ifndef DHT_H
#define DHT_H

#include "application.h"

#define MAXTIMINGS 85

#define DHT11 11
#define DHT22 22
#define DHT21 21
#define AM2301 21

#define NAN 999999

class DHT {
    private:
        uint8_t _data[6];
        uint8_t _pin, _type, _count;
        unsigned long _lastReadTime;
        bool _firstReading;

        bool read(void);

    public:
        DHT(uint8_t pin, uint8_t type, uint8_t count = 6);

        void begin(void);

        float readTemperature(bool farenheit = false);
        float readHumidity(void);

        float computeHeatIndex(float temperature, float percentHumidity, bool isFahrenheit);

        float convertCtoF(float);
        float convertFtoC(float f);
};

#endif