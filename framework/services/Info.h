#ifndef INFO_H
#define INFO_H

#include "Service.h"

extern const char INFO_NAME[];

class Info : public Service<INFO_NAME> {
public:
    Info(Device* const device);
    virtual ~Info();

    String getInfo();
    virtual void onStateChanged(const State& state);

private:
    void publish();

    void onHttp_Info(HttpRequest &request, HttpResponse &response);

    Timer timer;
    Device* device;

    uint32_t startupTime;
    uint32_t connectTime;
};


#endif
