#ifndef BMP180_SENSOR_H
#define BMP180_SENSOR_H

#include "Feature.h"
#include <Libraries/BMP180/BMP180.h>


template<const char* const name, const int scl_gpio = 14, const int sda_gpio = 12>
class BmpSensor : public Feature<name> {
    
protected:
    BMP180 barometer;

public:

    BmpSensor(Device* device) :
            Feature<name>(device) {
        
        Wire.begin(scl_gpio, sda_gpio);

        if(!barometer.EnsureConnected())
            Serial.println("Could not connect to BMP180.");
        else {

            barometer.Initialize();
            barometer.PrintCalibrationData();

            this->updateTimer.initializeMs(10000, TimerDelegate(&BmpSensor::publishCurrentState, this));
            this->updateTimer.start(/*repeating:*/true);
        }
    }

    virtual void registerSubscriptions() {
    }


protected:
    void publishCurrentState() {
        if(!barometer.EnsureConnected()){
            Serial.println("Could not connect to BMP180.");
            return;
        }
        long currentPressure = barometer.GetPressure();
        debug_d("currentPressure: %l", currentPressure);
        float currentTemperature = barometer.GetTemperature();
        debug_d("currentTemperature: %f", currentTemperature);

        this->publish("pressure", String(currentPressure), true);
        this->publish("temperature", String(currentTemperature), true);
    }

private:

    Timer updateTimer;
};


#endif
