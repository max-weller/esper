#include "Device.h"


ServiceBase::ServiceBase() {
}

ServiceBase::~ServiceBase() {
}


String calculateTopicBase() {
    char topicBase[sizeof(MQTT_REALM) + 6 + 1];
    sprintf(topicBase, "%s/%06x", MQTT_REALM, system_get_chip_id());
    return String(topicBase);
}



const String Device::TOPIC_BASE = calculateTopicBase();

Device::Device() :
        wifiConnectionManager(WifiConnectionManager::StateChangedCallback(&Device::onWifiStateChanged, this)),
        mqttConnectionManager(MqttConnectionManager::StateChangedCallback(&Device::onMqttStateChanged, this),
                              MqttConnectionManager::MessageCallback(&Device::onMqttMessageReceived, this)),
        ntpClient(NtpTimeResultDelegate(&Device::onTimeUpdated, this)),

#if HEARTBEAT_ENABLED
        heartbeat(this),
#endif
#if UPDATE_ENABLED
        update(this),
#endif
        info(this) {

#if HEARTBEAT_ENABLED
    this->add(&heartbeat);
#endif
#if UPDATE_ENABLED
    this->add(&update);
#endif
    this->add(&info);

    debug_d("Initialized");
    debug_d("Base Path: %s", Device::TOPIC_BASE.c_str());
}

Device::~Device() {
}

void Device::start() {
    debug_d("Starting");

    this->wifiConnectionManager.connect();

    debug_d("Started");
}

void Device::reboot() {
    debug_w("Restarting System");
    System.restart();
}

void Device::registerSubscription(const String& topic, const MessageCallback& callback) {
    this->messageCallbacks[topic] = callback;
}

void Device::add(ServiceBase* const service) {
    if (!services.contains(service)) {
        services.addElement(service);
        debug_i("Added service: %s", service->getName());
    }
}

void Device::publish(const String& topic, const String& message, const bool& retain) {
    if (this->mqttConnectionManager.getState() != MqttConnectionManager::State::CONNECTED)
        return;

    this->mqttConnectionManager.publish(topic,
                                        message,
                                        retain);
}

void Device::onWifiStateChanged(const WifiConnectionManager::State& state) {
    switch (state) {
        case WifiConnectionManager::State::CONNECTED: {
            debug_i("WiFi state changed: Connected");
            this->ntpClient.requestTime();
            this->mqttConnectionManager.connect();
            break;
        }

        case WifiConnectionManager::State::DISCONNECTED: {
            debug_i("WiFi state changed: Disconnected");
            break;
        }

        case WifiConnectionManager::State::CONNECTING: {
            debug_i("WiFi state changed: Connecting");
            break;
        }
    }
}

void Device::onMqttStateChanged(const MqttConnectionManager::State& state) {
    switch (state) {
        case MqttConnectionManager::State::CONNECTED: {
            debug_i("MQTT state changed: Connected \\o/");

            // Subscribe for all known registered callbacks
            for (unsigned int i = 0; i < this->messageCallbacks.count(); i++) {
                const auto topic = this->messageCallbacks.keyAt(i);
                this->mqttConnectionManager.subscribe(topic);
            }

            // Inform all services about new state
            for (int i = 0; i < this->services.count(); i++) {
                this->services[i]->onStateChanged(ServiceBase::State::CONNECTED);
            }

            break;
        }

        case MqttConnectionManager::State::DISCONNECTED: {
            debug_i("MQTT state changed: Disconnected");

            // Inform all services about new state
            for (int i = 0; i < this->services.count(); i++) {
                this->services[i]->onStateChanged(ServiceBase::State::DISCONNECTED);
            }

            break;
        }

        case MqttConnectionManager::State::CONNECTING: {
            debug_i("MQTT state changed: Connecting");
            break;
        }
    }
}

void Device::onMqttMessageReceived(const String& topic, const String& message) {
    // Dispatch message to registered feature handler
    auto i = this->messageCallbacks.indexOf(topic);
    if (i != -1) {
        const auto& callback = this->messageCallbacks.valueAt(i);
        callback(topic.substring(Device::TOPIC_BASE.length()+1), message);
    }
}

void Device::onTimeUpdated(NtpClient& client, time_t curr) {
    auto prev = RTC.getRtcSeconds();
    RTC.setRtcSeconds(curr);

    debug_i("Time updated: %s", DateTime(curr).toISO8601().c_str());

    if (abs(curr - prev) > 60 * 60) {
        debug_e("Clock differs to much - rebooting");
        this->reboot();
    }
}


void init() {
    // Configure system
    System.setCpuFrequency(eCF_160MHz);

    Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
    Serial.systemDebugOutput(true); // Debug output to serial

    const rboot_config rbootconf = rboot_get_config();

    debug_i("");
    debug_i("");
    debug_i("SDK: v%s", system_get_sdk_version());
    debug_i("Boot: v%u (%u)", system_get_boot_version(), system_get_boot_mode());
    debug_i("ESPER: v%u", VERSION);
    debug_i("Free Heap: %d", system_get_free_heap_size());
    debug_i("CPU Frequency: %d MHz", system_get_cpu_freq());
    debug_i("System Chip ID: %x", system_get_chip_id());
    debug_i("SPI Flash ID: %x", spi_flash_get_id());
    debug_i("ROM Selected: %d", rbootconf.current_rom);
    debug_i("ROM Slot 0: %08X", rbootconf.roms[0]);
    debug_i("ROM Slot 1: %08X", rbootconf.roms[1]);
    debug_i("Device: %x", DEVICE);
    debug_i("");
    debug_i("");

    Device* device = createDevice();
    device->start();
}
