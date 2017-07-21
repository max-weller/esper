#include "Device.h"
#include <stdio.h>


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

    this->mqttConnectionManager.setWill(Device::TOPIC_BASE + "/status", "OFFLINE", 0, true);
    this->wifiConnectionManager.connect();

#ifdef HTTP_PORT
	http.listen(HTTP_PORT);
	http.addPath("/", HttpPathDelegate(&Device::onHttp_Index, this));
#endif

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

            // Set Online Status
            this->mqttConnectionManager.publish(Device::TOPIC_BASE + "/status", "ONLINE", true);

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

#ifdef HTTP_PORT
void Device::onHttp_Index(HttpRequest &request, HttpResponse &response)
{
    response.setContentType(ContentType::TEXT);
    response.setHeader("Server", "ham");
	char s[128];
    const rboot_config rbootconf = rboot_get_config();
                sprintf(s, "Device: %s\n", DEVICE);
    response.sendString(s);
                sprintf(s, "SDK: v%s\n", system_get_sdk_version());
    response.sendString(s);
                sprintf(s, "Boot: v%u (%u)\n", system_get_boot_version(), system_get_boot_mode());
    response.sendString(s);
                sprintf(s, "ESPER: v%s\n", VERSION);
    response.sendString(s);
                sprintf(s, "Free Heap: %d\n", system_get_free_heap_size());
    response.sendString(s);
                sprintf(s, "CPU Frequency: %d MHz\n", system_get_cpu_freq());
    response.sendString(s);
                sprintf(s, "System Chip ID: %x\n", system_get_chip_id());
    response.sendString(s);
                sprintf(s, "SPI Flash ID: %x\n", spi_flash_get_id());
    response.sendString(s);
                sprintf(s, "ROM Selected: %d\n", rbootconf.current_rom);
    response.sendString(s);
                sprintf(s, "ROM Slot 0: %08X\n", rbootconf.roms[0]);
    response.sendString(s);
                sprintf(s, "ROM Slot 1: %08X\n", rbootconf.roms[1]);
    response.sendString(s);
                sprintf(s, "Time: %d\n", RTC.getRtcSeconds());
    response.sendString(s);
                sprintf(s, "IP: %s\n", WifiStation.getIP().toString().c_str() );
    response.sendString(s);
                sprintf(s, "MAC: %s\n", WifiStation.getMAC().c_str() );
    response.sendString(s);

    for (int i = 0; i < this->services.count(); i++) {
                sprintf(s, "Service: %s\n", this->services[i]->getName() );
        response.sendString(s);
    }

    for (unsigned int i = 0; i < this->messageCallbacks.count(); i++) {
                sprintf(s, "Endpoint: %s\n", this->messageCallbacks.keyAt(i).c_str() );
        response.sendString(s);
    }
}
#endif

void init() {
    // Configure system
    System.setCpuFrequency(eCF_160MHz);

    Device* device = createDevice();
    device->start();
}
