#ifndef DS1820SENSOR_H
#define DS1820SENSOR_H

#include "Feature.h"

#include <Libraries/OneWire/OneWire.h>

#include <Libraries/DS18S20/ds18s20.h>


template<const char* const name, int gpio>
class Ds1820Sensor : public Feature<name> {
    
protected:
    DS18S20 Sensor;

public:

    Ds1820Sensor(Device* device) :
            Feature<name>(device) {
        
        Sensor.Init(gpio);
        Sensor.StartMeasure();

        this->updateTimer.initializeMs(60000, TimerDelegate(&Ds1820Sensor::publishCurrentState, this));
        this->updateTimer.start(/*repeating:*/true);
    }

    virtual void registerSubscriptions() {
    }


protected:
    void publishCurrentState() {
        char idstr[42];
        if (!Sensor.MeasureStatus())  {// the last measurement completed
        
            //debug_i("******************************************");
            debug_i("Reading temperature ...");
            for(int a=0;a<Sensor.GetSensorsCount();a++) {  // prints for all sensors
                
                uint64_t id=Sensor.GetSensorID(a);
                sprintf(idstr, "%08X-%08X/temperature", (uint32_t)(id>>32), (uint32_t)id);
                if (Sensor.IsValidTemperature(a)) {  // temperature read correctly ?
                    float temperature = Sensor.GetCelsius(a);
                    this->publish(idstr, String(temperature), true);

                    debug_i("* Temperature sensor nr=%d, id=%s, temperature=%f", a, idstr, temperature);
                } else
                    debug_w("* Temperature sensor nr=%d, id=%s, Temperature not valid", a, idstr);

            }
            //debug_i("******************************************");
            Sensor.StartMeasure();  // next measure, result after 1.2 seconds * number of sensors
        }
        else
            debug_w("No valid Measure so far! wait please");
    }

private:

    Timer updateTimer;
};


#endif
