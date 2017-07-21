#ifndef DEVICE_H
#define DEVICE_H

#include <SmingCore/SmingCore.h>

#include "managers/WifiConnectionManager.h"
#include "managers/MqttConnectionManager.h"

#include "services/Info.h"

#if HEARTBEAT_ENABLED
#include "services/Heartbeat.h"
#endif

#if UPDATE_ENABLED
#include "services/Update.h"
#endif


class Device {

    Device(Device const&) = delete;
    Device& operator=(Device const&) = delete;

public:
    using MessageCallback = MqttConnectionManager::MessageCallback;

    static const String TOPIC_BASE;

    Device();
    virtual ~Device();

    void start();

    void reboot();

    void registerSubscription(const String& topic, const MessageCallback& callback);

    void publish(const String &topic, const String &message, const bool& retain = false);

    void add(ServiceBase* const service);
    
protected:
    void onHttp_Index(HttpRequest &request, HttpResponse &response);

#ifdef HTTP_PORT
    HttpServer http;
#endif

private:
    void onWifiStateChanged(const WifiConnectionManager::State& state);

    void onMqttStateChanged(const MqttConnectionManager::State& state);

    void onMqttMessageReceived(const String& topic, const String& message);

    void onTimeUpdated(NtpClient& client, time_t time);

    WifiConnectionManager wifiConnectionManager;
    MqttConnectionManager mqttConnectionManager;

    NtpClient ntpClient;

    Vector<ServiceBase*> services;

    HashMap<String, MessageCallback> messageCallbacks;

#if HEARTBEAT_ENABLED
    Heartbeat heartbeat;
#endif

#if UPDATE_ENABLED
    Update update;
#endif

    Info info;
};


Device* createDevice();

#endif
