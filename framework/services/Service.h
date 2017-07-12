#ifndef SERVICE_H
#define SERVICE_H

class Device;


class ServiceBase {
public:
    enum class State {
        CONNECTED,
        DISCONNECTED
    };

protected:
    explicit ServiceBase();
    virtual ~ServiceBase();

public:
    virtual const char* getName() const = 0;

    virtual void onStateChanged(const State& state) = 0;
};


template<const char* const name>
class Service : public ServiceBase {
protected:

    Service(Device* const device) :
            device(device) {
    }

    virtual ~Service() {
    }

public:
    virtual const char* getName() const {
        return name;
    }

    virtual void onStateChanged(const Service::State& state) {
    }

protected:
    Device* const device;
};



#endif
