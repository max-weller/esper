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

enum class PropertyDataType {
    Undefined,
    Integer,
    Float,
    Boolean,
    String,
    Enum,
    Command
};

class NodeProperty {
public:
    MqttConnectionManager::MessageCallback setCallback;
    PropertyDataType dataType;
    String displayName;
    String format;
    String unit;
    NodeProperty() {}
    NodeProperty(const MqttConnectionManager::MessageCallback& callback, PropertyDataType dataType, const String& displayName, 
        const String& format = "", const String& unit = "") :
        setCallback(callback), dataType(dataType), displayName(displayName), format(format), unit(unit) {
    }
    String dataTypeString() const {
        switch(this->dataType) {
        case PropertyDataType::Undefined: return "";
        case PropertyDataType::Integer: return "integer";
        case PropertyDataType::Float: return "float";
        case PropertyDataType::Boolean: return "boolean";
        case PropertyDataType::String: return "string";
        case PropertyDataType::Enum: return "enum";
        case PropertyDataType::Command: return "CMD";
        }
    }
};

class Device {

    Device(Device const&) = delete;
    Device& operator=(Device const&) = delete;

public:
    using MessageCallback = MqttConnectionManager::MessageCallback;

    static const String TOPIC_BASE;

    Device();
    virtual ~Device();

    virtual void start();

    void reboot();

    void registerProperty(const String& topic, const NodeProperty& prop);

    void publish(const String &topic, const String &message, const bool& retain = false);

    void add(ServiceBase* const service);

    const WifiConnectionManager& getWifi() const;
    const MqttConnectionManager& getMqtt() const;

    const Vector<ServiceBase*>& getServices() const;
    const HashMap<String, NodeProperty>& getSubscriptions() const;

private:
    void onWifiStateChanged(const WifiConnectionManager::State& state);

    void onMqttStateChanged(const MqttConnectionManager::State& state);

    void onMqttMessageReceived(const String& topic, const String& message);

    void onTimeUpdated(NtpClient& client, time_t time);

    WifiConnectionManager wifiConnectionManager;
    MqttConnectionManager mqttConnectionManager;

    NtpClient ntpClient;

    Vector<ServiceBase*> services;

    HashMap<String, NodeProperty> messageCallbacks;

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
